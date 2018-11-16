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

void closeUselessFd(int stderr[][2], int stdout[][2], int i, int num_procs) {
  int j;
  for (j = 0; j<num_procs; j++) {
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
<<<<<<< HEAD
    newargv[0] = malloc(strlen("ssh"));
=======
    char *pwd=malloc(100);
    getcwd(pwd,100);

    newargv[0] = malloc(strlen("ssh"));
    newargv[2] = malloc(1000);
>>>>>>> 8b8c2d022a38808d6e9697f638074389180456df
    strcpy(newargv[0],"ssh");
    strcpy(newargv[2],pwd);
    strcat(newargv[2],argv[1]);

<<<<<<< HEAD
    char * pwd = malloc(100);
    getcwd(pwd, 100);

    newargv[2] = malloc(strlen(pwd) + strlen(argv[1]) + 5);
    sprintf(newargv[2], "%s/bin/%s", pwd,argv[1]);
    printf("%s\n", newargv[2]);
    fflush(stdout);

=======
>>>>>>> 8b8c2d022a38808d6e9697f638074389180456df
    for (i=3; i<argc + 1; i++){
        newargv[i] = malloc(strlen(argv[i-1]));
        strcpy(newargv[i],argv[i-1]);
    }
<<<<<<< HEAD

    newargv[argc+1] = NULL;
    free(pwd);
=======
    // newargv[argc+1] = malloc(10);
    newargv[argc+1]=NULL;
>>>>>>> 8b8c2d022a38808d6e9697f638074389180456df
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
<<<<<<< HEAD
     int nbRead;
     char * buffer = malloc(100);
=======
     printf("sock = %i\n", sock);
     char *buffer=malloc(1000);
>>>>>>> 8b8c2d022a38808d6e9697f638074389180456df
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

     char * newargv[argc+2];
     createNewArgv(newargv, argv, argc);

     for(i = 0; i < num_procs ; i++) {
       memset(buffer,0,sizeof(buffer));
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
<<<<<<< HEAD

     updateNewargv(newargv,machines,i);
     printf("%s\n", newargv[2]);
     execvp("ssh",newargv);
     break;
=======
     updateNewargv(newargv,machines,i);
>>>>>>> 8b8c2d022a38808d6e9697f638074389180456df

	   execvp("ssh",newargv);
   break;
	} else  if(pid > 0) { /* pere */
    close(stderr[i][1]);
    close(stdout[i][1]);
<<<<<<< HEAD

    memset(buffer, '\0', strlen(buffer));
    nbRead = read(stdout[i][0], buffer, 100);
    printf("%s\n", buffer);

=======
    read(stdout[i][0],buffer,1000);
    printf("%s\n",buffer);
>>>>>>> 8b8c2d022a38808d6e9697f638074389180456df
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
