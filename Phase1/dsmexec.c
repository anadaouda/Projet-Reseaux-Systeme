#include "common_impl.h"

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

void closeUselessFd(int stderr[][2], int stdout[][2], int i, int num_procs) {
  for (int j = 0; j<num_procs; j++) {
    close(stderr[j][0]);
    close(stdout[j][0]);
    if (j != i) {
      close(stderr[j][1]);
      close(stdout[j][1]);
    }
  }
}

void createNewArgv(char * newargv[], char * argv [], int argc) {
    int i;
    newargv[0] = malloc(strlen("ssh"));

    strcpy(newargv[0],"ssh");

    for (i=2; i<argc + 1; i++){
        newargv[i] = malloc(strlen(argv[i-1]));
        strcpy(newargv[i],argv[i-1]);
    }
}

void updateNewargv(char * newargv[], char* machines[], int i) {
    newargv[1] = malloc(strlen(machines[i]));
    strcpy(newargv[1],machines[i]);
}

int main(int argc, char *argv[])
{
  if (argc < 3){
    usage();
  } else {
     pid_t pid;
     int num_procs = nbMachines("machine_file");;
     int i;
     printf("procs %i\n", num_procs);
     char * machines[num_procs];
     nomMachines("machine_file", machines);

     int sock = createSocket();
     printf("sock = %i\n", sock);
     /* Mise en place d'un traitant pour recuperer les fils zombies*/
     /* XXX.sa_handler = sigchld_handler; */

     /* lecture du fichier de machines */
     /* 1- on recupere le nombre de processus a lancer */
     /* 2- on recupere les noms des machines : le nom de */
     /* la machine est un des elements d'identification */



     /* creation de la socket d'ecoute */
     /* + ecoute effective */

     /* creation des fils */
     int stderr[num_procs][2];
     int stdout[num_procs][2];

     char * newargv[argc+1];
     createNewArgv(newargv, argv, argc);

     for(i = 0; i < num_procs ; i++) {

	/* creation du tube pour rediriger stdout */
  pipe(stderr[i]);
  pipe(stdout[i]);

	/* creation du tube pour rediriger stderr */

	pid = fork();
	if(pid == -1) ERROR_EXIT("fork");

	if (pid == 0) { /* fils */

	   /* redirection stdout */
     close(STDOUT_FILENO);
     dup(stdout[i][1]);
     close(stdout[i][1]);
     close(stdout[i][0]);

	   /* redirection stderr */
     close(STDERR_FILENO);
     dup(stderr[i][1]);
     close(stderr[i][1]);
     close(stderr[i][0]);

     closeUselessFd(stderr, stdout, i, num_procs);
	   /* Creation du tableau d'arguments pour le ssh */

	   /* jump to new prog : */
     //createNewargv()
     //ssh localhost truc arguments
     updateNewargv(newargv,machines,i);
	 execvp("ssh",newargv);

	} else  if(pid > 0) { /* pere */
    close(stderr[i][1]);
    close(stdout[i][1]);
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
     /* sur les tubes de redirection de stdout/stderr */
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
