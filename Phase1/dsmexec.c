#include "common_impl.h"

/* Variables globales */

dsm_proc_t *proc_array = NULL; // un tableau gerant les infos d'identification des processus dsm
int ** newStderr = NULL;
int ** newStdout = NULL;
char * machines[MAX_HOSTNAME];
int sock;
volatile int procsCreated = 0; // le nombre de processus effectivement crees

// Cette fonction permet de lire/afficher les information des tubes de redirection
void * pipeRead(void * args) {
    pipeReadArgs_t arguments = *(pipeReadArgs_t *)args;
    int nbProcs = arguments.nbProcs;
    int ** newStderr = arguments.newStderr;
    int ** newStdout = arguments.newStdout;
    char ** machines = arguments.machines;

    /* Initialisation de la structure poll :
			0 -> nbProcs-1 : newStdout
			nbProcs -> 2*nbProcs -1 : newStderr
		*/
    struct pollfd pollStd[nbProcs*2];
    memset(pollStd, '0', sizeof(struct pollfd)*(nbProcs)*2);
    for (int i = 0; i < nbProcs; i++) {
        pollStd[i].fd = newStdout[i][0];
        pollStd[i].events = POLLIN|POLLHUP;

        pollStd[i + nbProcs].fd = newStderr[i][0];
        pollStd[i + nbProcs].events = POLLIN|POLLHUP;
    }

		// Récuperation des infos des tubes de redirection
    char * buffer = malloc(MAX_BUFFER_SIZE);
		int leave = 0; // Variable gerant la sortie du thread
    while(leave < nbProcs) {
			if (poll(pollStd, nbProcs*2, -1) > 0) {
				for (int j = 0; j < nbProcs*2; j++) {
            if(pollStd[j].revents == POLLIN) {
                memset(buffer, '\0', MAX_BUFFER_SIZE);
                read(pollStd[j].fd, buffer, MAX_BUFFER_SIZE);
								
                if (j >= nbProcs) {
                    printf("[Proc %i : %s : %s] %s\n", j-nbProcs, machines[j-nbProcs], "stderr", buffer);
										fflush(stdout);
                } else {
                    printf("[Proc %i : %s : %s] %s\n", j, machines[j], "stdout", buffer);
										fflush(stdout);
                }
            } else if (pollStd[j].revents == POLLHUP) {
              leave++;
            }
        }
			}
    }

		free(buffer);
    return NULL;
}

void usage(void) {
  fprintf(stdout,"Usage : dsmexec machine_file executable arg1 arg2 ...\n");
  fflush(stdout);
  exit(EXIT_FAILURE);
}

void freeEverything(char * buffer, dsm_proc_t * proc_array, char * machines[], int ** newStderr, int **newStdout, int sock) {
  for (int i = 0; i < procsCreated; i++) {
			free(machines[i]);
      free(newStdout[i]);
      free(newStderr[i]);
      close((proc_array + i)->connect_info.comSock);
		}
    close(sock);
    free(newStderr);
    free(newStdout);
    free(proc_array);
    free(buffer);
}
// Traintant de signal des zombies
void sigchld_handler(int sig) {
    waitpid(-1,NULL,0);
    printf("bye\n");
    fflush(stdout);
}

// Ferme les extrémités inutiles des tubes
void closeUselessFd(int ** newStderr, int ** newStdout, int rank, int nbProcs) {
  for (int j = 0; j < rank + 1; j++) {
    close(newStderr[j][0]);
    close(newStdout[j][0]);
    if (j != rank) {
      close(newStderr[j][1]);
      close(newStdout[j][1]);
    }
  }
}

// Crée la nouvelle liste des arguments qui sera utilisé par le programme dsmwrap
void createNewArgv(char * newArgv[], char * argv [], int argc, int dsmExecPort) {
    char * pwd = malloc(100);
		char execCommand[] = "ssh";
		char sshCommand[] = "dsmwrap";

		getcwd(pwd, 100);
		
		newArgv[0] = malloc(strlen(execCommand)+1);
		newArgv[1] = malloc(MAX_HOSTNAME);
		newArgv[2] = malloc(strlen(pwd) + strlen(sshCommand) + 6); // 6 correspond a la taille de /bin/ + 1 pour \0
    newArgv[3] = malloc(MAX_HOSTNAME);
		newArgv[4] = malloc(6); //6 correspond au nombre maximum de chiffre dans le dsmExecPort ie 5 +1 pour \0
		newArgv[5] = malloc(2); // 2 correspond au nombre de chiffre du rang +1 pour \0
		newArgv[6] = malloc(strlen(pwd) + strlen(argv[2]) + 6); // 6 correspond a la taille de /bin/ + 1 pour \0

		strcpy(newArgv[0], execCommand);
    sprintf(newArgv[2], "%s/bin/%s", pwd,sshCommand);
		gethostname(newArgv[3], MAX_HOSTNAME);
		sprintf(newArgv[4], "%i", dsmExecPort);
    sprintf(newArgv[6], "%s/bin/%s", pwd,argv[2]);

    for (int i = 7; i < argc + 4; i++) {
        newArgv[i] = malloc(strlen(argv[i-4])+1);
        strcpy(newArgv[i],argv[i-4]);
    }

    newArgv[argc + 4] = NULL;
    free(pwd);
}

// Ajoute le nom de la machine et son rang à la liste des arguments pour le programme dsmwrap
void updateNewargv(char * newArgv[], char* machines[], int i) {
    memset(newArgv[1], '\0', MAX_HOSTNAME);

    strcpy(newArgv[1], machines[i]);
		sprintf(newArgv[5], "%i", i);
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
		usage();
  } else {
		srand(time(NULL));
     int nbProcs, port, i;
		 struct sockaddr_in sockAddr;
		 struct sigaction action;
		 char * newArgv[argc + 5];

		 nbProcs = nbMachines(argv[1]); // recuperation du nombre de processus a lancer

		 
		 int ** newStderr = malloc(nbProcs*sizeof(int *)); // tubes de redirection de la sortie d'erreur
		 int ** newStdout = malloc(nbProcs*sizeof(int *)); // tubes de redirection de la sortie standard
     nomMachines(argv[1], machines); // recuperation des noms de machines

     sock = createSocket(&sockAddr, &port); // socket d'ecoute
     
		 // Mise en place du traitant de signal
     memset(&action, 0, sizeof(struct sigaction));
     action.sa_handler = sigchld_handler;
     sigaction(SIGCHLD, &action, NULL);
		 
     createNewArgv(newArgv, argv, argc, port);
		 /*
		 argv = {dsmexec, machine_file, programmeALancer, argumentsDuProgramme..., NULL}
		 newArgv = {ssh, nomMachine, dsmwrap, nomHote, portDsmExec, rang, programmeALancer, argumentsDuProgramme..., NULL}
		 => newArgc = argc + 5
		 */
  
     for(int i = 0; i < nbProcs ; i++) {
			 pid_t pid;
			 newStderr[i] = malloc(2*sizeof(int));
			 newStdout[i] = malloc(2*sizeof(int));
			 
			 pipe(newStderr[i]);
			 pipe(newStdout[i]);

	pid = fork();
	if(pid == -1) ERROR_EXIT("fork");

	if (pid == 0) { /* fils */
	   /* redirection newStdout */
     close(STDOUT_FILENO);
     dup(newStdout[i][1]);
     close(newStdout[i][1]);
     close(newStdout[i][0]);

	   /* redirection newStderr */
     close(STDERR_FILENO);
     dup(newStderr[i][1]);
     close(newStderr[i][1]);
     close(newStderr[i][0]);

     closeUselessFd(newStderr, newStdout, i, nbProcs);
     updateNewargv(newArgv,machines,i);
		 execvp("ssh",newArgv);

     break;
	} else  if (pid > 0) { /* pere */
  /* fermeture des extremites des tubes non utiles */
    close(newStderr[i][1]);
    close(newStdout[i][1]);

	   procsCreated++;
	  }
  }

for(int i = 0; i < argc + 4; i++) {
      free(newArgv[i]);
}


pthread_t pipeRd;
pipeReadArgs_t args = {newStderr, newStdout, machines, nbProcs};
pthread_create(&pipeRd, NULL, pipeRead, (void *)&args);

char * buffer = malloc(MAX_BUFFER_SIZE);
dsm_proc_t *proc_array = malloc(sizeof(dsm_proc_t)*procsCreated);
int acceptSock;

// Remplissage du tableau proc_array
for(i = 0; i < procsCreated ; i++){

  acceptSock = do_accept(sock, sockAddr);
	do_receive(acceptSock, buffer);

	int procport, procPid, procRank;
  char name[MAX_HOSTNAME];

	sscanf(buffer, "%s %i %i %i", name, &procRank, &procport, &procPid);

	dsm_proc_t newProc = {name, procPid, {procRank, acceptSock, procport}};
	*(proc_array + i) = newProc;

	sprintf(buffer, "%i", procsCreated);

  // Envoie du nombre de processus dsm
	do_send(buffer, acceptSock);
}

// Affichage des données du tableau proc_array
//printProcArray(proc_array, procsCreated);

memset(buffer, '\0', MAX_BUFFER_SIZE);

// Propagation des données dsm à tous les processus dsm
for (int i = 0; i < procsCreated; i++) {
	for (int j = 0; j < procsCreated; j++) {
		sprintf(buffer, "%s %i %i %i %i",
		(proc_array + i)->name,
		(proc_array + i)->pid,
		(proc_array + i)->connect_info.rank,
		(proc_array + i)->connect_info.comSock,
		(proc_array + i)->connect_info.port);
		do_send(buffer, (proc_array + j)->connect_info.comSock);
	}
}

     /* gestion des E/S : on recupere les caracteres */
     /* sur les tubes de redirection de newStdout/newStderr */
     /* while(1)
         {
            je recupere les infos sur les tubes de redirection
            jusqu'à ce qu'ils soient inactifs (ie fermes par les
            processus dsm ecrivains de l'autre cote ...)

         };
      */

     /* on attend les processus fils */

     /* on ferme les descripteurs proprement */

     /* on ferme la socket d'ecoute */
    
    pthread_join(pipeRd, NULL);
		int waitPid;
		while ((waitPid = wait(NULL)) > 0);
		freeEverything(buffer, proc_array, machines, newStderr, newStdout, sock);

  }

   exit(EXIT_SUCCESS);
}
