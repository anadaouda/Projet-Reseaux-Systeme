#include "common_impl.h"

/* variables globales */

/* un tableau gerant les infos d'identification */
/* des processus dsm */
dsm_proc_t *proc_array = NULL;
int sock;
int acceptSock;
/* le nombre de processus effectivement crees */
volatile int num_procs_creat = 0;

struct pipeReadArgs {
    int ** newStderr;
    int ** newStdout;
    char ** machines;
    int num_procs;
};


void * pipeRead(void * args) {
    struct pipeReadArgs arguments = *(struct pipeReadArgs *)args;
    int num_procs = arguments.num_procs;
    int ** newStderr = arguments.newStderr;
    int ** newStdout = arguments.newStdout;
    char ** machines = arguments.machines;


    //Initialisation poll
    struct pollfd pollStd[num_procs*2];
    memset(pollStd, '0', sizeof(struct pollfd)*(num_procs)*2);
    int i;
    for (i = 0; i < num_procs; i++) {
        pollStd[i].fd = newStdout[i][0];
        pollStd[i].events = POLLIN;

        pollStd[i+num_procs].fd = newStderr[i][0];
        pollStd[i+num_procs].events = POLLIN;
    }

    char * buffer = malloc(MAX_BUFFER_SIZE);

    while(1) {
        int resPoll = poll(pollStd, num_procs*2, -1);
        int j;
        for (j = 0; j < num_procs*2; j++) {
            if(pollStd[j].revents == POLLIN) {
                memset(buffer, '\0', MAX_BUFFER_SIZE);
                read(pollStd[j].fd, buffer, MAX_BUFFER_SIZE);
								
                if (j >= num_procs) {
                    printf("[Proc %i : %s : %s] %s\n", j-num_procs, machines[j-num_procs], "stderr", buffer);
                } else {
                    printf("[Proc %i : %s : %s] %s\n", j, machines[j], "stdout", buffer);
                }
                fflush(stdout);
            }
        }
    }
    return NULL;
}

void usage(void)
{
  fprintf(stdout,"Usage : dsmexec machine_file executable arg1 arg2 ...\n");
  fflush(stdout);
  exit(EXIT_FAILURE);
}

void sigchld_handler(int sig)
{
   char text[] = "Bye\n";
   write(STDOUT_FILENO, text, sizeof(text));
   close(sock);
   exit(0);
}

void closeUselessFd(int ** newStderr, int ** newStdout, int i, int num_procs) {
  int j;
  for (j = 0; j < num_procs; j++) {
    close(newStderr[j][0]);
    close(newStdout[j][0]);
    if (j != i) {
      close(newStderr[j][1]);
      close(newStdout[j][1]);
    }
  }
}

void createNewArgv(char * newargv[], char * argv [], int argc, int port) {
    int i;
    newargv[0] = malloc(strlen("ssh"));
    strcpy(newargv[0], "ssh");

    newargv[1] = malloc(MAX_HOSTNAME);

    char * pwd = malloc(100);
    getcwd(pwd, 100);

    newargv[2] = malloc(strlen(pwd) + strlen("dsmwrap") + 6);
    newargv[3] = malloc(MAX_HOSTNAME);
		newargv[4] = malloc(sizeof(int));
		newargv[5] = malloc(sizeof(int));

    gethostname(newargv[3], MAX_HOSTNAME);
    newargv[6] = malloc(strlen(pwd) + strlen(argv[2]) + 6);
    sprintf(newargv[2], "%s/bin/%s", pwd,"dsmwrap");
		sprintf(newargv[4], "%i", port);
    sprintf(newargv[6], "%s/bin/%s", pwd,argv[2]);

    for (i = 7; i < argc + 4; i++) {
        newargv[i] = malloc(strlen(argv[i-4])+1);
        strcpy(newargv[i],argv[i-4]);
    }

    newargv[argc + 4] = NULL;
    free(pwd);
}

void updateNewargv(char * newargv[], char* machines[], int i) {
    memset(newargv[1], '\0', MAX_HOSTNAME);
    strcpy(newargv[1],machines[i]);
		sprintf(newargv[5], "%i", i);
}

int main(int argc, char *argv[])
{
  if (argc < 3){
  } else {
		srand(time(NULL));
     pid_t pid;
     int num_procs = nbMachines(argv[1]);;
     int i;

     char * machines[num_procs];
     nomMachines(argv[1], machines);
     struct sockaddr_in sockDsmAddr;
     int port;
     sock = createSocket(&sockDsmAddr, &port);
     struct sigaction action;
     memset(&action, 0, sizeof(struct sigaction));
     action.sa_handler = sigchld_handler;
     sigaction(SIGINT, &action, NULL);
		 

     // faire un thread qui va s'occuper de faire les accept

     /* Mise en place d'un traitant pour recuperer les fils zombies*/
     /* XXX.sa_handler = sigchld_handler; */

     /* lecture du fichier de machines */
     /* 1- on recupere le nombre de processus a lancer */
     /* 2- on recupere les noms des machines : le nom de */
     /* la machine est un des elements d'identification */



     /* creation de la socket d'ecoute */
     /* + ecoute effective */

     /* creation des fils */
     //int newStderr[num_procs][2];
     //int newStdout[num_procs][2];

     int ** newStderr = malloc(num_procs*sizeof(int *));
     int ** newStdout = malloc(num_procs*sizeof(int *));

     char * newargv[argc + 5];
     // argv = dsmexec machine_file truc hlo hela heli (null)
     // argc = argc mais size(argv) = argc + 1
     createNewArgv(newargv, argv, argc, port);
     // newargv = argv - dsmexec - machine_file + ssh + localhost + dsmwrap + hostname + port + rank
		 // newargC = argc - 2 + 5 = argc + 4
		 // donc size(newargv ) = argc + 5
     // newargv = ssh localhost dsmwrap hostname port rank truc helo hela heli

     for(i = 0; i < num_procs ; i++) {

	/* creation du tube pour rediriger newStdout */
    newStderr[i] = malloc(2*sizeof(int));
    newStdout[i] =  malloc(2*sizeof(int));

    pipe(newStderr[i]);
    pipe(newStdout[i]);

	/* creation du tube pour rediriger newStderr */

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

     //closeUselessFd(newStderr, newStdout, i, num_procs);
	   /* Creation du tableau d'arguments pour le ssh */
	   /* jump to new prog : */

     updateNewargv(newargv,machines,i);
		 
     execvp("ssh",newargv);
     wait(NULL);

     break;
	} else  if (pid > 0) { /* pere */
    close(newStderr[i][1]);
    close(newStdout[i][1]);
	   /* fermeture des extremites des tubes non utiles */
	   num_procs_creat++;
	  }
    }

pthread_t pipeRd;
struct pipeReadArgs args = {newStderr, newStdout, machines, num_procs};
pthread_create(&pipeRd, NULL, pipeRead, (void *)&args);
    //faire le poll ici parce que on sait que tout les processus sont fait.
    // {newstdout, newstderr, machines}




//dsm_proc_t * dsm_proc_id = malloc(num_procs*sizeof(dsm_proc_t));
char * buffer = malloc(MAX_BUFFER_SIZE);
dsm_proc_t *proc_array = malloc(sizeof(dsm_proc_t)*num_procs_creat);

for(i = 0; i < num_procs_creat ; i++){

  acceptSock = do_accept(sock, sockDsmAddr);
	do_receive(acceptSock, buffer);

	char * name = malloc(MAX_HOSTNAME);
	int procPort, procPid, procRank;

	sscanf(buffer, "%s %i %i %i", name, &procRank, &procPort, &procPid);

	dsm_proc_t newProc = {name, procPid, {procRank, acceptSock, procPort}};
	*(proc_array + i) = newProc;
	
	sprintf(buffer, "%i", num_procs_creat);
	do_send(buffer, acceptSock);
	/* on accepte les connexions des processus dsm */

	/*  On recupere le nom de la machine distante */
	/* 1- d'abord la taille de la chaine */
	/* 2- puis la chaine elle-meme */

	/* On recupere le pid du processus distant  */

	/* On recupere le numero de port de la socket */
	/* d'ecoute des processus distants */

  //int comSock = do_accept();

  //do_read(name);
  //do_read(pid);
  //do_read(port);

  //int rank = get_rank(name, proc, machines);
  //dsm_proc_id[i] = {pid, {rank, port}};
     }

		 printProcArray(proc_array, num_procs_creat);

int j;
memset(buffer, '\0', MAX_BUFFER_SIZE);

for (i = 0; i < num_procs_creat; i++) {
	for (j = 0; j < num_procs_creat; j++) {
		sprintf(buffer, "%s %i %i %i %i",
		(proc_array + i)->name,
		(proc_array + i)->pid,
		(proc_array + i)->connect_info.rank,
		(proc_array + i)->connect_info.comSock,
		(proc_array + i)->connect_info.port);
		do_send(buffer, (proc_array + j)->connect_info.comSock);
	}
}

     /* envoi du nombre de processus aux processus dsm*/

     /* envoi des rangs aux processus dsm */

     /* envoi des infos de connexion aux processus */

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
  }

   exit(EXIT_SUCCESS);
}
