#include "common_impl.h"

void createNewArgv(char * newargv[], char ** argv, int argc) {
    int i;

    for (i = 0; i < argc-1; i++){
        newargv[i] = malloc(strlen(argv[i+4])+1);
        strcpy(newargv[i],argv[i+4]);
    }

    newargv[argc-1] = NULL;
}


int procIndex(dsm_proc_t * proc_array, int rank, int num_procs) {

    for(int i = 0; i < num_procs; i++) {
        if ((proc_array + i)->connect_info.rank == rank) {
            return i;
        }

    }

    return -1;
}

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

        printf("A recu la connexion du processus %i \n", atoi(processRank));
        fflush(stdout);
    }

    free(processRank);

    return NULL;
}


int main(int argc, char **argv)
{
    //argv = {path_to_dsmwrap, hostname, port, path_to_truc, arg1, args2, arg3, NULL};
    //argc est censé etre égal a 7;

    int nbArgs, rank, portEcoute, sock, sockEcoute, num_proc, procPid, procRank, comSock, procPort;
    for (nbArgs = 0; argv[nbArgs] != NULL; nbArgs++);

    struct addrinfo * dsmInfo = NULL;
    struct sockaddr_in sockEcouteAddr;

    dsm_proc_t *proc_array = NULL;
    pthread_t interProcessCoThread;
    interProcessCoArgs_t args;

    char * newargv[nbArgs-2];

    char * buffer = malloc(MAX_BUFFER_SIZE);
    char * hostname = malloc(MAX_HOSTNAME);
    char * procName = malloc(MAX_HOSTNAME);
    char * rankStr = malloc(MAX_BUFFER_SIZE);

    dsmInfo = get_addr_info(argv[1], atoi(argv[2]));
    sock = do_connect(dsmInfo);

   /* Envoi des informations au lanceur */
   gethostname(hostname, MAX_HOSTNAME); // nom d'hote
   rank = atoi(argv[3]);
   sockEcoute = createSocket(&sockEcouteAddr, &portEcoute);

   sprintf(buffer, "%s %i %i %i", argv[1],rank, portEcoute, getpid());
   do_send(buffer, sock);

   /* Reception du nombre de processus */
   do_receive(sock,buffer);
   sscanf(buffer, "%i", &num_proc);

   /* Reception des informations des autres processus */
   proc_array = malloc(sizeof(dsm_proc_t)*num_proc);

   for(int i = 0; i < num_proc; i++) {

       do_receive(sock,buffer);
       sscanf(buffer, "%s %i %i %i %i", procName,&procPid,&procRank,&comSock,&procPort);

       dsm_proc_t newProc = {hostname, procPid, {procRank, comSock, procPort}};
       *(proc_array + i) = newProc;
   }
   //printProcArray(proc_array, num_proc);

/* Connexion inter-processus */
    /* Initialisation des variables */

   args.sock = sockEcoute;
   args.rank = rank;
   args.num_procs = num_proc;
   args.sockAddr = sockEcouteAddr;
   args.proc_array = proc_array;
   

   /* Creation du thread */
   pthread_create(&interProcessCoThread, NULL, interProcessCo, (void *)&args);

   /* Connexion aux autres processus */
    //Chaque processus va se connecter aux processus de rang supérieur
    
    

   for (int i = 0; i < rank; i++) {
       int index = procIndex(proc_array, i, num_proc);

       sprintf(rankStr, "%i", rank);
       
       dsmInfo = get_addr_info(proc_array[i].name, proc_array[i].connect_info.port);
       
       proc_array[index].connect_info.comSock = do_connect(dsmInfo);
       do_send(rankStr, proc_array[index].connect_info.comSock);

       printf("S'est connecte au processus %i \n", i);
       fflush(stdout);
   }
   
   pthread_join(interProcessCoThread, NULL);


/*
for (int i = 0; i < num_proc; i++) {
    printf("%i ", (proc_array + i)->connect_info.comSock);
    fflush(stdout);
    close((proc_array + i)->connect_info.comSock);
}
*/

printf("\n%i %i", sock, sockEcoute);
fflush(stdout);

//close((proc_array)->connect_info.comSock);
close(sock);
   close(sockEcoute);

    free(rankStr);
   free(procName);
   free(proc_array);

   createNewArgv(newargv, argv, nbArgs-3);
   execvp(newargv[0],newargv);
   return 0;
}
