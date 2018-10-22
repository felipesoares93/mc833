#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>
#include <strings.h>
#include <arpa/inet.h>

#include "basic.h"
#include "socket_helper.h"

<<<<<<< HEAD
#define LISTENQ 10
#define MAXDATASIZE 4096
=======
// #define LISTENQ 10              
#define MAXDATASIZE 4096         
>>>>>>> arthur
#define EXIT_COMMAND "exit\n"

void doit(int connfd, struct sockaddr_in clientaddr);

int main (int argc, char **argv) {
<<<<<<< HEAD
   int    listenfd,
          connfd,
          port;
   struct sockaddr_in servaddr;
   char   error[MAXDATASIZE + 1];
=======
   int    listenfd,              
          connfd,               
          port,
          listenq;                  
   struct sockaddr_in servaddr;  
   char   error[MAXDATASIZE + 1];     
>>>>>>> arthur

   if (argc != 3) {
      strcpy(error,"uso: ");
      strcat(error,argv[0]);
      strcat(error," <Port, Backlog>");
      perror(error);
      exit(1);
   }

   port = atoi(argv[1]);
   listenq = atoi(argv[2]);

   listenfd = Socket(AF_INET, SOCK_STREAM, 0);

   servaddr = ServerSockaddrIn(AF_INET, INADDR_ANY, port);

   Bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

   Listen(listenfd, listenq);


   for ( ; ; ) {
      pid_t pid;

      struct sockaddr_in clientaddr;
      socklen_t clientaddr_len = sizeof(clientaddr);

      connfd = Accept(listenfd, (struct sockaddr *) &clientaddr, &clientaddr_len);
      
      if (getpeername(connfd, (struct sockaddr *) &clientaddr, &clientaddr_len) == -1) {
         perror("getpeername() failed");
         return 0 ;
      }

      printf("<%s-%d>: connected\n", inet_ntoa(clientaddr.sin_addr),(int) ntohs(clientaddr.sin_port));
      
      sleep(5);
      
      if((pid = fork()) == 0) {
         
         Close(listenfd);

         doit(connfd, clientaddr);

         // printf("Fechou: %s - %d>\n", inet_ntoa(clientaddr.sin_addr),(int) ntohs(clientaddr.sin_port));

         Close(connfd);

         exit(0);
      }

      signal(SIGCHLD,SIG_IGN);

      Close(connfd);
   }

   return(0);
}

void doit(int connfd, struct sockaddr_in clientaddr) {
   char recvline[MAXDATASIZE + 1];
   char   message[MAXDATASIZE + 1];
   int n;
   socklen_t remoteaddr_len = sizeof(clientaddr);

   while ((n = read(connfd, recvline, MAXDATASIZE)) > 0) {
      recvline[n] = 0;

      if (getpeername(connfd, (struct sockaddr *) &clientaddr, &remoteaddr_len) == -1) {
         perror("getpeername() failed");
         return;
      }

      // printf("Digite uma mensagem:\n");
      fgets(message, MAXDATASIZE, stdin);
      if(strcmp(message, EXIT_COMMAND) == 0) {
         break;
      }

      write(connfd, message, strlen(message));

   }
}
