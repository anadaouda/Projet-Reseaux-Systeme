#include "common_impl.h"

void createNewArgv(char * newargv[], char ** argv, int argc) {
    int i;
    for (i = 0; i < argc-1; i++){
        newargv[i] = malloc(strlen(argv[i+4])+1);
        strcpy(newargv[i],argv[i+4]);
    }
    newargv[argc-1] = NULL;
}



int main(int argc, char **argv)
{
    //argv = {path_to_dsmwrap, hostname, port, path_to_truc, arg1, args2, arg3, NULL};
    //argc est censé etre égal a 7;

    int nbArgs;
    for (nbArgs = 0; argv[nbArgs] != NULL; nbArgs++);
    
    char * newargv[nbArgs-2];

    struct addrinfo * dsmInfo = get_addr_info(argv[1], atoi(argv[2]));
    int sock = do_connect(dsmInfo);

   /* processus intermediaire pour "nettoyer" */
   /* la liste des arguments qu'on va passer */
   /* a la commande a executer vraiment */

   // Il faut les infos de la socket pour que ca marche
   /* creation d'une socket pour se connecter au */
   /* au lanceur et envoyer/recevoir les infos */
   /* necessaires pour la phase dsm_init */

   /* Envoi du nom de machine au lanceur */
   char * buffer = malloc(MAX_BUFFER_SIZE);
   char * hostname = malloc(MAX_HOSTNAME);
   gethostname(hostname, MAX_HOSTNAME);

   struct sockaddr_in sockEcouteAddr;
   int portEcoute;
   int sockEcoute = createSocket(&sockEcouteAddr, &portEcoute);

   sprintf(buffer, "%s %i %i %i", argv[1],atoi(argv[3]), portEcoute, getpid());
   

   do_send(buffer, sock);
   do_receive(sock,buffer);

   int num_proc;
   sscanf(buffer, "%i", &num_proc);
   dsm_proc_t *proc_array = malloc(sizeof(dsm_proc_t)*num_proc);
   
   int procPid, procRank, comSock, procPort;
   char * procName = malloc(MAX_HOSTNAME);

   for(int i = 0; i < num_proc; i++) {
       do_receive(sock,buffer);
       sscanf(buffer, "%s %i %i %i %i", procName,&procPid,&procRank,&comSock,&procPort);
       dsm_proc_t newProc = {hostname, procPid, {procRank, comSock, procPort}};
       *(proc_array + i) = newProc;
   }

   free(procName);
   free(proc_array);
   /* Envoi du pid au lanceur */

   /* Creation de la socket d'ecoute pour les */
   /* connexions avec les autres processus dsm */

   /* Envoi du numero de port au lanceur */
   /* pour qu'il le propage à tous les autres */
   /* processus dsm */


   
   /* on execute la bonne commande */
   createNewArgv(newargv, argv, nbArgs-3);
   close(sock);
   close(sockEcoute);
   execvp(newargv[0],newargv);

   return 0;
}
