#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>

#include "basic.h"
#include "socket_helper.h"

#define MAXLINE 4096
#define EXIT_COMMAND "exit\n"    


void doit(int sockfd);

int main(int argc, char **argv) {
   int    port, sockfd;                  
   char * ip;                      
   char   error[MAXLINE + 1];       
   struct sockaddr_in servaddr;    

   if (argc != 3) {
      strcpy(error,"uso: ");
      strcat(error,argv[0]);
      strcat(error," <IPaddress, Port>");
      perror(error);
      exit(1);
   }

  
   ip = argv[1];
   port = atoi(argv[2]);

   sockfd = Socket(AF_INET, SOCK_STREAM, 0);

   servaddr = ClientSockaddrIn(AF_INET, ip, port);

   Connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

   doit(sockfd);

   exit(0);
}

void doit(int sockfd) {
   char   message[MAXLINE + 1];     
   char   response[MAXLINE + 1];   
   int    n;                      

   printf("Digite uma mensagem:\n");
   fgets (message, MAXLINE, stdin);

   write(sockfd, message, strlen(message));
   
   while((n = read(sockfd, response, MAXLINE)) > 0) {
      response[n] = 0; 
      
      printf("Resposta do servidor: %s\n", response);

      printf("Digite uma mensagem:\n");
      fgets (message, MAXLINE, stdin);

      if(strcmp(message, EXIT_COMMAND) == 0) {
         break;
      }

      write(sockfd, message, strlen(message));
   }
}
