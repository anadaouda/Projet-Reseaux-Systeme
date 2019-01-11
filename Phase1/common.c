#include "common_impl.h"


int randInt(int min, int max) {
    int result = (rand() % (max - min)) + min;
    return result;
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

void init_serv_addr(struct sockaddr_in * sock_addr, int port) {

  memset(sock_addr, '\0', sizeof(struct sockaddr_in));
  sock_addr->sin_family = AF_INET;
  sock_addr->sin_port = htons(port);
  sock_addr->sin_addr.s_addr = htonl(INADDR_ANY);

}

struct addrinfo * get_addr_info(char * hostname, int port) {
    int status;
    struct addrinfo hints;
    struct addrinfo * res = malloc(sizeof(struct addrinfo));
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype=SOCK_STREAM;

		char portChar[7];
		sprintf(portChar, "%i", ntohs(port));
    if ((status = getaddrinfo(hostname,portChar,&hints,&res)) == -1) {
        perror("Getaddrinfo");
        exit(EXIT_FAILURE);
    }

    return res;
}

int do_connect(struct addrinfo * res) {
    struct addrinfo * p;
    int sock = do_socket();
    for (p = res; p !=NULL; p = p->ai_next) {
        if(connect(sock, p->ai_addr, p->ai_addrlen) !=-1) {
            return sock;
        }
    }
    perror("Connect");
    free(res);
    return -1;
}

void do_bind(int sock, struct sockaddr_in * sock_addr) {
  if (bind(sock, (struct sockaddr *) sock_addr, sizeof(struct sockaddr_in)) == -1) {
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

int createSocket(struct sockaddr_in * sockDsmAddr, int *port) {
    int sockDsm = do_socket();
		do {
			int randPort = randInt(1024, 65535);
			init_serv_addr(sockDsmAddr, randPort);
		} while ( bind(sockDsm, (struct sockaddr *) sockDsmAddr, sizeof(struct sockaddr_in)) == -1);
    do_listen(sockDsm);
    *port = sockDsmAddr->sin_port;
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
  char * str = malloc(MAX_HOSTNAME);

  while(fgets(str, MAX_HOSTNAME, file) != NULL) {
      text[wordCount] = malloc(MAX_HOSTNAME);
      if(str[strlen(str)-1] == '\n') {
          str[strlen(str)-1] = '\0';
      }
			if (!strcmp(str, "localhost")) {
				gethostname(str, MAX_HOSTNAME);
			}
      strcpy(text[wordCount], str);
      wordCount++;
  }
  fclose(file);
  free(str);
  return;
}

void do_send(char * buffer, int sock) {
    int strSent;
    int inputLen;

    inputLen = strlen(buffer);

    do {
        strSent = send(sock,&inputLen,sizeof(int),0);
        if (strSent == -1) {
            perror("Send");
            exit(EXIT_FAILURE);
        }
    } while (strSent != sizeof(int));

    do {
        strSent = send(sock,buffer,strlen(buffer),0);
        if (strSent == -1) {
            perror("Send");
            exit(EXIT_FAILURE);
        }
    } while (strSent != strlen(buffer));
    memset(buffer, '\0', MAX_BUFFER_SIZE);
}


void do_receive(int sock, char * buffer) {
    memset(buffer, '\0', MAX_BUFFER_SIZE);
        int strReceived, strSizeToReceive;
        do {
            strReceived = recv(sock, &strSizeToReceive, sizeof(int), 0);
            if (strReceived == -1) {
                perror("Receive");
                exit(EXIT_FAILURE);
            }
        } while (strReceived != sizeof(int));


        do {
            strReceived = recv(sock, buffer, strSizeToReceive, 0);
            if (strReceived == -1) {
                perror("Receive");
                exit(EXIT_FAILURE);
            }
        } while (strReceived != strSizeToReceive);
}

void addProc (dsm_proc_t * p_dsmProc, int index, int dsmProcSize, char * hostname, int pid, int rank, int comSock, int port) {
  if (index < dsmProcSize) {
		dsm_proc_t newProc = {hostname, pid, {rank, comSock, port}};
		*(p_dsmProc + index) = newProc;
	}
}

void printArgs (char* args[], int nbArgs) {
	int i;

	for (i = 0; i < nbArgs; i++) {
		printf("%s ", args[i]);
	}
	printf("\n");
	fflush(stdout);
}

void printProcArray(dsm_proc_t * proc_array, int num_procs) {
	int i;
	
	for( i = 0; i < num_procs; i++) {
		printf("%s :\n- pid %i\n- rank %i\n- comSock %i\n- port %i\n\n",
		(proc_array + i)->name,
		(proc_array + i)->pid,
		(proc_array + i)->connect_info.rank,
		(proc_array + i)->connect_info.comSock,
		(proc_array + i)->connect_info.port);
	}
	fflush(stdout);
}

/* Vous pouvez ecrire ici toutes les fonctions */
/* qui pourraient etre utilisees par le lanceur */
/* et le processus intermediaire. N'oubliez pas */
/* de declarer le prototype de ces nouvelles */
/* fonctions dans common_impl.h */
