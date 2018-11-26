#include "common_impl.h"
#include <unistd.h>
/* variables globales */

/* un tableau gerant les infos d'identification */
/* des processus dsm */
dsm_proc_t *proc_array = NULL;

/* le nombre de processus effectivement crees */
volatile int num_procs_creat = 0;

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

void closeUselessFd(int newStderr[][2], int newStdout[][2], int i, int num_procs) {
  int j;
  for (j = 0; j<num_procs; j++) {
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

    newargv[2] = malloc(strlen(pwd) + strlen("dsmwrap") + 5);
    newargv[3] = malloc(strlen(pwd) + strlen(argv[2]) + 5);
    sprintf(newargv[2], "%s/bin/%s", pwd,"dsmwrap");
    sprintf(newargv[3], "%s/bin/%s", pwd,argv[2]);

    for (i = 4; i < argc + 1; i++){
        newargv[i] = malloc(strlen(argv[i-1]));
        strcpy(newargv[i],argv[i-1]);
    }

    newargv[argc + 1] = NULL;
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
     printf("procs %i\n", num_procs);
     char * machines[num_procs];
     nomMachines(argv[1], machines);
     //int sock = createSocket();
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
     int newStderr[num_procs][2];
     int newStdout[num_procs][2];

     char * newargv[argc + 2]; // +2 parce que il y a un argument de plus et pour le NULL
     createNewArgv(newargv, argv, argc);

     for(i = 0; i < num_procs ; i++) {

	/* creation du tube pour rediriger newStdout */
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

     closeUselessFd(newStderr, newStdout, i, num_procs);
	   /* Creation du tableau d'arguments pour le ssh */
	   /* jump to new prog : */
       
     updateNewargv(newargv,machines,i);
     execvp("ssh",newargv);

     break;
	} else  if(pid > 0) { /* pere */
    close(newStderr[i][1]);
    close(newStdout[i][1]);

    memset(buffer, '\0', strlen(buffer));
    nbRead = read(newStdout[i][0], buffer, 100);
    printf("%s\n", buffer);

	   /* fermeture des extremites des tubes non utiles */
	   num_procs_creat++;
	  }
    }

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
