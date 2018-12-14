#include "common_impl.h"

int creer_socket(int prop, int *port_num)
{
   int fd = 0;

   /* fonction de creation et d'attachement */
   /* d'une nouvelle socket */
   /* renvoie le numero de descripteur */
   /* et modifie le parametre port_num */

   return fd;
}

int do_socket() {
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  int yes = 1;

  if (sock == -1) {
    perror("Socket");
    exit(EXIT_FAILURE);
  }

  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("ERROR setting socket options");
  }

  return sock;
}

struct sockaddr_in * init_serv_addr() {
  struct sockaddr_in * sock_addr = malloc(sizeof(struct sockaddr_in));

  memset(&sock_addr, '\0', sizeof(sock_addr));
  sock_addr.sin_family = AF_INET;
  sock_addr->sin_port = htons(0);
  sock_addr->sin_addr.s_addr = htonl(INADDR_ANY);

  return sock_addr;
}

struct addrinfo * get_addr_info(char * hostname) {
    int status;
    struct addrinfo hints;
    struct addrinfo * res = malloc(sizeof(struct addrinfo));
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype=SOCK_STREAM;
    if ((status = getaddrinfo(hostname,NULL,&hints,&res)) == -1) {
        perror("Getaddrinfo");
        exit(EXIT_FAILURE);
    }

    return res;
}

int do_connect(struct addrinfo * res) {
    struct addrinfo * p;
    int sock = do_socket();
    for (p = res; p !=NULL; p = p->ai_next) {
        if(connect(sock, p->ai_addr, p->ai_addrlen)!=-1) {
            return sock;
        }
    }
    perror("Connect");
    return -1;
}

void do_bind(int sock, struct sockaddr_in * sock_addr) {
  if (bind(sock, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) == -1) {
    perror("Bind");
    exit(EXIT_FAILURE);
  }
}

void do_listen(int sock) {
  if (listen(sock, SOMAXCONN) == -1) {
    perror("Listen");
    exit(EXIT_FAILURE);
  }
}

int do_accept(int sock, struct sockaddr_in sock_addr) {
  int addrlen = sizeof(struct sockaddr);
  int rdwr_socket = accept(sock, (struct sockaddr *) &sock_addr, (socklen_t *)&addrlen);
  if (rdwr_socket == -1) {
    perror("Accept");
    exit(EXIT_FAILURE);
  }
  return rdwr_socket;
}

int createSocket(struct sockaddr_in * sockDsmAddr) {
    int sockDsm = do_socket();
    sockDsmAddr = init_serv_addr();
    do_bind(sockDsm, sockDsmAddr);
    do_listen(sockDsm);
    return sockDsm;
}

int nbMachines(char * path) {
  FILE * file = fopen(path, "r");

  int wordCount = 0, wordLetterCount = 0, nbLines = 0;
  char letter;

  do {
    letter = fgetc(file);

    if (letter == '\n') {
      nbLines++; wordCount++; wordLetterCount = 0;
    }
    else if (letter == ' ') {
      wordCount++; wordLetterCount = 0;
    }
    else {
      wordLetterCount++;
    }

  } while(!feof(file));

  fclose(file);

  return nbLines;
}

void nomMachines(char * path, char ** text) {
  FILE * file = fopen(path, "r");

  int wordCount = 0;
  char * str = malloc(20);

  while(fgets(str, 20, file) != NULL) {
      text[wordCount] = malloc(20);
      if(str[strlen(str)-1] == '\n') {
          str[strlen(str)-1] = '\0';
      }
      strcpy(text[wordCount], str);
      wordCount++;
  }
  fclose(file);

  return;
}
/* Vous pouvez ecrire ici toutes les fonctions */
/* qui pourraient etre utilisees par le lanceur */
/* et le processus intermediaire. N'oubliez pas */
/* de declarer le prototype de ces nouvelles */
/* fonctions dans common_impl.h */
