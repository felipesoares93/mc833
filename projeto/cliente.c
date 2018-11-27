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
#include <time.h>

#include "basic.h"
#include "socket_helper.h"

#define MAXDATASIZE 4096
#define EXIT_COMMAND "exit\n"


void doit(int sockfd);

int main(int argc, char **argv) {
   int    port, sockfd;
   char * ip;
   char   error[MAXDATASIZE + 1];
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
   char   message[MAXDATASIZE + 1];
   char   response[MAXDATASIZE + 1];
   int    n;

   while((n = read(sockfd, response, MAXDATASIZE)) > 0) {
      response[n] = 0;

      printf("%s\n", response);

      if (strcmp(response,"\nVocê é o carrasco\nQual palavra quer utilizar?\n") == 0) {
        printf("Aguardando jogadores...");
        sleep(5);
        write(sockfd, message, strlen(message));
      } else {
        printf("Cliente: ");
        fgets (message, MAXDATASIZE, stdin);

        if(strcmp(message, EXIT_COMMAND) == 0) {
           break;
        }

        write(sockfd, message, strlen(message));
      }
   }
}
