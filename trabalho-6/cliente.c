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
// #include "unp.h"

#define MAXLINE 4096
#define EXIT_COMMAND "exit\n"
#define max(a,b) ((a) > (b) ? (a) : (b))  


void doit(FILE *fp_in, int sockfd);

int main(int argc, char **argv) {
   int    port, sockfd;                  
   char * ip;                      
   char   error[MAXLINE + 1];       
   struct sockaddr_in servaddr;
   FILE *fp_in = stdin;  

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

   doit(fp_in, sockfd);

   exit(0);
}

void doit(FILE *fp_in, int sockfd) {
   char     message[MAXLINE + 1],     
            response[MAXLINE + 1],
            *line = NULL;   
   int      n,
            maxfdp1,
            stdineof;
   fd_set   rset;
   size_t   len = 0;
   ssize_t  r; 


   stdineof = 0;
   FD_ZERO(&rset);
   int c = 0;
   for ( ; ; ) {
      c = c+1;
      if (stdineof == 0) {
         printf("%d aqui ",c);
         FD_SET(fileno(fp_in), &rset);
      }
      FD_SET(sockfd, &rset);
      maxfdp1 = max(fileno(fp_in), sockfd) + 1;
      select(maxfdp1, &rset, NULL, NULL, NULL);

      if (c == 20)
         break;
      
      if (FD_ISSET(sockfd, &rset)) {
         if (read(sockfd, response, MAXLINE) == 0) {
            printf("%d FD_ISSET S Y %s\n", c, response);
            if (stdineof == 1) {
               continue;
            } else {
               printf("client: server terminated prematurely\n");
               exit(0);
            }
         }
         printf("FD_ISSET S N %s\n", response);
         fputs(response, stdout);
      }

      if (FD_ISSET(fileno(fp_in), &rset)) {
         if ((r = getline(&line, &len, fp_in)) == -1) {
         // if (fgets(message, MAXLINE, fp_in) == NULL) {
            printf("FD_ISSET FP Y %s\n", line);
            stdineof = 1;
            shutdown(sockfd, 1);
            FD_CLR(fileno(fp_in), &rset);
            continue;
         }
         printf("FD_ISSET FP N %s\n", line);
         write(sockfd, line, len);
      }
   }                   

   // printf("Digite uma mensagem:\n");
   // fgets (message, MAXLINE, stdin);

   // char *line = NULL;
   // size_t len = 0;
   // ssize_t read; 
   // FILE *fp_in = stdin;
   // FILE *fp_out = stdout;
   // while ((read = getline(&line, &len, fp_in)) != -1) {
   //    fprintf(fp_out, "%s", line);
   // }

   // fclose(fp_in);
   // fclose(fp_out);
   // if (line) {
   //    free(line);
   // }

   // write(sockfd, message, strlen(message));
   
   // while((n = read(sockfd, response, MAXLINE)) > 0) {
   //    response[n] = 0; 
      
   //    printf("Resposta do servidor: %s\n", response);

   //    printf("Digite uma mensagem:\n");
   //    fgets (message, MAXLINE, stdin);

   //    if(strcmp(message, EXIT_COMMAND) == 0) {
   //       break;
   //    }

   //    write(sockfd, message, strlen(message));
   // }
}
