
#include "common_impl.h"

void createNewArgv(char * newargv[], char ** argv, int argc) {
    int i;

    for (i = 0; i < argc-1; i++) {
        newargv[i] = malloc(strlen(argv[i+4])+1);
        strcpy(newargv[i],argv[i+4]);
    }

    newargv[argc-1] = NULL;
}

void freeEverything(int num_proc, dsm_proc_t * proc_array, int sock, int sockEcoute, char * rankStr) {

    for (int i = 0; i < num_proc; i++) {
        free((proc_array + i)->name);
        close((proc_array + i)->connect_info.comSock);
    }


    close(sock);
    close(sockEcoute);

    free(proc_array);
    free(rankStr);

}

// Retourne l'index du processus de rang rank dans le tableau proc_array
int procIndex(dsm_proc_t * proc_array, int rank, int num_procs) {

    for(int i = 0; i < num_procs; i++) {
        if ((proc_array + i)->connect_info.rank == rank) {
            return i;
        }
    }

    return -1;
}

// Thread permettant d'accepter les connexions du processus
void * interProcessCo(void * args) {
    // Chaque processus va accepter les connexions des processus de rang inferieur
    interProcessCoArgs_t arguments = *(interProcessCoArgs_t *)args;
    int sock = arguments.sock;
    int rank = arguments.rank;
    int num_procs = arguments.num_procs;
    dsm_proc_t * proc_array = arguments.proc_array;

    struct sockaddr_in sockAddr;
    char * processRank = malloc(MAX_BUFFER_SIZE);


    for (int i = rank + 1; i < num_procs; i++) {
        int acceptSock = do_accept(sock, sockAddr);

        do_receive(acceptSock,processRank);

        int index = procIndex(proc_array, atoi(processRank), num_procs);

        addProc(proc_array,
                index,
                num_procs,
                (proc_array + index)->name,
                (proc_array + index)->pid,
                i,
                acceptSock,
                (proc_array + index)->connect_info.port);

        printf("A accepte la connexion du processus %i \n", atoi(processRank));
        fflush(stdout);
    }

    free(processRank);

    return NULL;
}


int main(int argc, char ** argv) {

    //argv = {dsmwrap, nomHoteLanceur, programmeALancer, argumentsDuProgramme..., NULL}

    /* VARIABLES*/
    int nbArgs, rank, portEcoute, sock, sockEcoute, num_proc, procPid, procRank, comSock, procPort;
    for (nbArgs = 0; argv[nbArgs] != NULL; nbArgs++);

    struct addrinfo * dsmInfo = NULL;
    struct addrinfo * processInfo = NULL;
    struct sockaddr_in sockEcouteAddr;

    dsm_proc_t *proc_array = NULL;
    pthread_t interProcessCoThread;
    interProcessCoArgs_t args;

    char * newargv[nbArgs-2];

    char * buffer = malloc(MAX_BUFFER_SIZE);
    char * hostname = malloc(MAX_HOSTNAME);
    char * rankStr = malloc(MAX_BUFFER_SIZE);

    /* CONNEXION AU LANCEUR */
    dsmInfo = get_addr_info(argv[1], atoi(argv[2]));
    sock = do_connect(dsmInfo);

    /* ENVOIE DES INFORMATIONS AU LANCEUR */
    gethostname(hostname, MAX_HOSTNAME); // nom d'hote
    rank = atoi(argv[3]);
    sockEcoute = createSocket(&sockEcouteAddr, &portEcoute);

    sprintf(buffer, "%s %i %i %i", hostname, rank, portEcoute, getpid());
    do_send(buffer, sock);

    /* RECEPTION DU NOMBRE DE PROCESSUS */
    do_receive(sock,buffer);
    sscanf(buffer, "%i", &num_proc);

    /* RECEPTION DES INFORMATION DES AUTRES PROCESSUS */
    proc_array = malloc(sizeof(dsm_proc_t)*num_proc);

    for(int i = 0; i < num_proc; i++) {
        char * procName = malloc(MAX_HOSTNAME);
        do_receive(sock,buffer);
        sscanf(buffer, "%s %i %i %i %i", procName,&procPid,&procRank,&comSock,&procPort);

        dsm_proc_t newProc = {procName, procPid, {procRank, comSock, procPort}};
        *(proc_array + i) = newProc;
    }

    printProcArray(proc_array, num_proc);

    /* CONNEXION INTER-PROCESSUS */
    /* INITIALISATION DES VARIABLES */

    args.sock = sockEcoute;
    args.rank = rank;
    args.num_procs = num_proc;
    args.sockAddr = sockEcouteAddr;
    args.proc_array = proc_array;


    /* CREATION DU THREAD  D'ACCEPTATION DE CONNEXION */
    pthread_create(&interProcessCoThread, NULL, interProcessCo, (void *)&args);

    /* CONNEXION AUX AUTRES PROCESSUS */
    //Chaque processus va se connecter aux processus de rang supérieur
    for (int i = 0; i < rank; i++) {
        int index = procIndex(proc_array, i, num_proc);
        processInfo = get_addr_info(proc_array[index].name, proc_array[index].connect_info.port);

        if ((proc_array[index].connect_info.comSock = do_connect(processInfo)) == -1) {
            fprintf(stderr, "N'a pas pu se connecter a %s \n", proc_array[index].name);
            fflush(stderr);
        } else {
            printf("S'est connecte au processus %s \n", proc_array[index].name);
            fflush(stdout);

            // Envoie du rang au processus auquel on se connecte
            sprintf(rankStr, "%i", rank);
            do_send(rankStr, proc_array[index].connect_info.comSock);
        }

    }

    /* SORTIE */
    pthread_join(interProcessCoThread, NULL);
    freeEverything(num_proc, proc_array, sock, sockEcoute, rankStr);

    /* LANCEMENT DU PROGRAMME A EFFECTIVEMENT EXECUTER */
    createNewArgv(newargv, argv, nbArgs-3);
    execvp(newargv[0],newargv);
    return 0;
}
