#include "common_impl.h"

void createNewArgv(char * newargv[], char ** argv, int argc) {
    int i;
    for (i = 0; i < argc-1; i++){
        newargv[i] = malloc(strlen(argv[i+2])+1);
        strcpy(newargv[i],argv[i+2]);
    }
    newargv[argc-1] = NULL;

}



int main(int argc, char **argv)
{
    //argv = {path_to_dsmwrap, hostname, path_to_truc, arg1, args2, arg3, NULL};
    //argc est censé etre égal a 5;


    int nbArgs;
    for (nbArgs = 0; argv[nbArgs] != NULL; nbArgs++);
    char * newargv[nbArgs-1];
    struct addrinfo * dsmInfo = get_addr_info(argv[1]);
    int sock = do_connect(dsmInfo);
    
   /* processus intermediaire pour "nettoyer" */
   /* la liste des arguments qu'on va passer */
   /* a la commande a executer vraiment */

   // Il faut les infos de la socket pour que ca marche
   /* creation d'une socket pour se connecter au */
   /* au lanceur et envoyer/recevoir les infos */
   /* necessaires pour la phase dsm_init */

   /* Envoi du nom de machine au lanceur */

   /* Envoi du pid au lanceur */

   /* Creation de la socket d'ecoute pour les */
   /* connexions avec les autres processus dsm */

   /* Envoi du numero de port au lanceur */
   /* pour qu'il le propage à tous les autres */
   /* processus dsm */

   /* on execute la bonne commande */
   createNewArgv(newargv, argv, nbArgs-1);
   execvp(newargv[0],newargv);
   while(1);

   return 0;
}
