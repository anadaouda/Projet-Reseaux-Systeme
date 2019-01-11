#include "common_impl.h"

/* Variables globales */
dsm_proc_t *proc_array = NULL; // un tableau gerant les infos d'identification des processus dsm
//int ** newStderr = NULL;
//int ** newStdout = NULL;
//char * machines[MAX_HOSTNAME];
int sock;

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

// Fonction permettant de liberer toute la memoire allouee et de fermer toutes les sockets ouvertes
void freeEverything(char * buffer, dsm_proc_t * proc_array, char * machines[], int ** newStderr, int **newStdout, int sock, int nbArgs, char * newArgv[], int nbProcs) {
    for (int i = 0; i < nbProcs; i++) {
        free(machines[i]);
        free(newStdout[i]);
        free(newStderr[i]);
        free((proc_array + i)->name);
        close((proc_array + i)->connect_info.comSock);
    }
    close(sock);
    free(newStderr);
    free(newStdout);
    free(proc_array);
    free(buffer);

    for(int i = 0; i < nbArgs; i++) {
        free(newArgv[i]);
    }
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
        srand(time(NULL)); // permet d'utiliser la fonction donnant un nombre aléatoire

        /* VARIABLES */
        int nbProcs, port, i;

        struct sockaddr_in sockAddr;
        struct sigaction action;

        char * newArgv[argc + 5];
        char * buffer = malloc(MAX_BUFFER_SIZE);

        /* RECUPERATION DU NOMBRE DE PROCESSUS A LANCER */
        nbProcs = nbMachines(argv[1]);

        char * machines[nbProcs];
        int ** newStderr = malloc(nbProcs*sizeof(int *)); // tubes de redirection de la sortie d'erreur
        int ** newStdout = malloc(nbProcs*sizeof(int *)); // tubes de redirection de la sortie standard
        dsm_proc_t * proc_array = malloc(sizeof(dsm_proc_t)*nbProcs);

        /* RECUPERATION DES NOMS DE MACHINE */
        nomMachines(argv[1], machines);

        /* SOCKET D'ECOUTE DU LANCEUR */
        sock = createSocket(&sockAddr, &port);

        /* MISE EN PLACE DU TRAITANT DE SIGNAL */
        memset(&action, 0, sizeof(struct sigaction));
        action.sa_handler = sigchld_handler;
        sigaction(SIGCHLD, &action, NULL);

        /* CREATION DU TABLEAU D'ARGUMENTS A PASSER A LA FONCTION EXEC */
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

            /* CREATION DES TUBES DE REDIRECTION */
            pipe(newStderr[i]);
            pipe(newStdout[i]);

            pid = fork();
            if(pid == -1) ERROR_EXIT("fork");

            if (pid == 0) { // fils
                /* REDIRECTION NEWSTDOUT */
                close(STDOUT_FILENO);
                dup(newStdout[i][1]);
                close(newStdout[i][1]);
                close(newStdout[i][0]);

                /* REDIRECTION NEWSTDERR */
                close(STDERR_FILENO);
                dup(newStderr[i][1]);
                close(newStderr[i][1]);
                close(newStderr[i][0]);

                closeUselessFd(newStderr, newStdout, i, nbProcs);
                updateNewargv(newArgv,machines,i);
                execvp("ssh",newArgv);

            } else  if (pid > 0) { // pere
                /* FERMETURE DES EXTREMITES INUTILES */
                close(newStderr[i][1]);
                close(newStdout[i][1]);
            }
        }


        /* CREATION DU THREAD DE LECTURE DES TUBES DE REDIRECTION */
        pthread_t pipeRd;
        pipeReadArgs_t args = {newStderr, newStdout, machines, nbProcs};
        pthread_create(&pipeRd, NULL, pipeRead, (void *)&args);

        /* REMPLISSAGE DU TABLEAU PROC_ARRAY */
        for(i = 0; i < nbProcs ; i++) {
            int procport, procPid, procRank, acceptSock;
            char * name = malloc(MAX_HOSTNAME);

            acceptSock = do_accept(sock, sockAddr);
            do_receive(acceptSock, buffer);

            sscanf(buffer, "%s %i %i %i", name, &procRank, &procport, &procPid);

            dsm_proc_t newProc = {name, procPid, {procRank, acceptSock, procport}};
            *(proc_array + i) = newProc;

            sprintf(buffer, "%i", nbProcs);

            /* ENVOIE DU NOMBRE DE PROCESSUS DSM */
            do_send(buffer, acceptSock);
        }
        /* AFFICHAGE DES DONNEES DU TABLEAU PROC_ARRAY */
        printProcArray(proc_array, nbProcs);

        /* PROPAGATION DES DONNEES DSM A TOUS LES PROCESSUS DSM */
        memset(buffer, '\0', MAX_BUFFER_SIZE);
        for (int i = 0; i < nbProcs; i++) {
            for (int j = 0; j < nbProcs; j++) {
                sprintf(buffer, "%s %i %i %i %i",
                        (proc_array + i)->name,
                        (proc_array + i)->pid,
                        (proc_array + i)->connect_info.rank,
                        (proc_array + i)->connect_info.comSock,
                        (proc_array + i)->connect_info.port);
                do_send(buffer, (proc_array + j)->connect_info.comSock);
            }
        }

        /* SORTIE */
        pthread_join(pipeRd, NULL);
        int waitPid;
        while ((waitPid = wait(NULL)) > 0) {
            printf("bye\n");
            fflush(stdout);
        }
        freeEverything(buffer, proc_array, machines, newStderr, newStdout, sock, argc + 4, newArgv, nbProcs);
    }
    exit(EXIT_SUCCESS);
}
