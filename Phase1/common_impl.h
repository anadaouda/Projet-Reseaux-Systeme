#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <limits.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <poll.h>
#include <sys/mman.h>

/* autres includes (eventuellement) */


#define MAX_BUFFER_SIZE 500
#define MAX_HOSTNAME 100

#define ERROR_EXIT(str) {perror(str);exit(EXIT_FAILURE);}

/* definition du type des infos */
/* de connexion des processus dsm */
struct dsm_proc_conn  {
   int rank;
   int comSock;
   int port;
   /* a completer */
};
typedef struct dsm_proc_conn dsm_proc_conn_t;

/* definition du type des infos */
/* d'identification des processus dsm */
struct dsm_proc {
  char * name;
  pid_t pid;
  dsm_proc_conn_t connect_info;
};
typedef struct dsm_proc dsm_proc_t;


struct pipeReadArgs {
    int ** newStderr;
    int ** newStdout;
    char ** machines;
    int nbProcs;
};
typedef struct pipeReadArgs pipeReadArgs_t;

int creer_socket(int type, int *port_num);
int nbMachines(char * path);
void nomMachines(char * path, char ** text);
int createSocket(struct sockaddr_in * sockDsmAddr, int* port);
int do_accept(int sock, struct sockaddr_in sock_addr);
struct addrinfo * get_addr_info(char * hostname, int port);
int do_connect(struct addrinfo * res);
void do_send(char * buffer, int sock);
void do_receive(int sock, char * buffer);
void addProc (dsm_proc_t * p_dsmProc, int index, int dsmProcSize, char * hostname, int pid, int rank, int comSock, int port);
void printArgs (char* args[], int nbArgs);
void printProcArray(dsm_proc_t * proc_array, int num_procs);