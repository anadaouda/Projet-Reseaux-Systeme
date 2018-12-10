#include "common_impl.h"
#include <unistd.h>
/* variables globales */

/* un tableau gerant les infos d'identification */
/* des processus dsm */
dsm_proc_t *proc_array = NULL;

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
    for (int i = 0; i < num_procs; i++) {
        pollStd[i].fd = newStdout[i][0];
        pollStd[i].events = POLLIN;

        pollStd[i+num_procs].fd = newStderr[i][0];
        pollStd[i+num_procs].events = POLLIN;
    }

    char * buffer = malloc(100);

    while(1) {
        poll(pollStd, num_procs, -1);
        for (int j = 0; j < num_procs*2; j++) {
            if(pollStd[j].revents == POLLIN) {
                memset(buffer, '\0', 100);
                read(newStdout[j][0], buffer, 100);
                if (j >= num_procs) {
                    printf("[Proc %i : %s : %s] %s\n", j, machines[j], "stderr", buffer);
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
   /* on traite les fils qui se terminent */
   /* pour eviter les zombies */
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

void createNewArgv(char * newargv[], char * argv [], int argc) {
    int i;
    newargv[0] = malloc(strlen("ssh"));
    strcpy(newargv[0], "ssh");

    newargv[1] = malloc(20);

    char * pwd = malloc(100);
    getcwd(pwd, 100);

    newargv[2] = malloc(strlen(pwd) + strlen("dsmwrap") + 6);
    newargv[3] = malloc(100);
    gethostname(newargv[3], 100);
    newargv[4] = malloc(strlen(pwd) + strlen(argv[2]) + 6);
    sprintf(newargv[2], "%s/bin/%s", pwd,"dsmwrap");
    sprintf(newargv[4], "%s/bin/%s", pwd,argv[2]);

    for (i = 5; i < argc + 2; i++) {
        newargv[i] = malloc(strlen(argv[i-2])+1);
        strcpy(newargv[i],argv[i-2]);
    }

    newargv[argc + 2] = NULL;
    free(pwd);
}

void updateNewargv(char * newargv[], char* machines[], int i) {
    memset(newargv[1], '\0', 20);
    strcpy(newargv[1],machines[i]);
}

int main(int argc, char *argv[])
{
  if (argc < 3){
    usage();
  } else {
     pid_t pid;
     int num_procs = nbMachines(argv[1]);;
     int i;

     char * machines[num_procs];
     nomMachines(argv[1], machines);

     int sock = createSocket();
     // faire un thread qui va s'occuper de faire les accept

     int nbRead;
     char * buffer = malloc(100);

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

     char * newargv[argc + 3]; // +2 parce que il y a un argument de plus et pour le NULL
     createNewArgv(newargv, argv, argc);
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

     break;
	} else  if(pid > 0) { /* pere */
    close(newStderr[i][1]);
    close(newStdout[i][1]);
	   /* fermeture des extremites des tubes non utiles */
	   num_procs_creat++;
	  }
    }

pthread_t pipeRd;
struct pipeReadArgs args = {newStderr, newStdout, machines, num_procs};
pthread_create(&pipeRd, NULL, pipeRead, (void *)&args);
pthread_join(pipeRd, NULL);
    //faire le poll ici parce que on sait que tout les processus sont fait.
    // {newstdout, newstderr, machines}

for(i = 0; i < num_procs ; i++){

	/* on accepte les connexions des processus dsm */

	/*  On recupere le nom de la machine distante */
	/* 1- d'abord la taille de la chaine */
	/* 2- puis la chaine elle-meme */

	/* On recupere le pid du processus distant  */

	/* On recupere le numero de port de la socket */
	/* d'ecoute des processus distants */
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
  }
   exit(EXIT_SUCCESS);
}
