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
#include <assert.h>

#include "basic.h"
#include "socket_helper.h"

#define MAXDATASIZE 4096
#define EXIT_COMMAND "exit\n"


void doit(int sockfd);
char** str_split(char* a_str, const char a_delim);

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
   char   message[MAXDATASIZE + 1], response[MAXDATASIZE + 1], response_mult[MAXDATASIZE + 1], temp_mult_line[MAXDATASIZE + 1], mult_players_ip[10][50];
   char** splitted_response;
   char** splitted_mult_info;
   int    n, mult_players_qty = 0, flag_read_mult_info = 0, flag_carrasco_waiting = 0, flag_carrasco_playing = 0, mult_players_port[10];

   while((n = read(sockfd, response, MAXDATASIZE)) > 0) {
      response[n] = 0;

      if (strcmp(response,"waiting_mult_game") != 0) {
        if (flag_carrasco_playing && strcmp(response,"\nO jogo não pode iniciar pois não há jogadores online!\n\n1) Iniciar partida simples\n2) Ser carrasco ao iniciar partida\n3) Jogar no modo multiplayer\n") != 0) {
          // Aqui, o cliente eh carrasco e a mensagem de volta nao foi de erro, logo o jogo comecou
          // vamos tratar a mensagem de volta para extrair os dados de ip/porta dos jogadores
          mult_players_qty = 0;
          sprintf(response_mult, "");

          splitted_response = str_split(response, '\n');
          if (splitted_response) {

            flag_read_mult_info = 1;
            for (int i = 0; *(splitted_response + i); i++) {
              if (flag_read_mult_info && strcmp(*(splitted_response + i), "O jogo multiplayer começou!") != 0) {

                strcpy(temp_mult_line, *(splitted_response + i));
                splitted_mult_info = str_split(temp_mult_line, '-');

                if (splitted_mult_info) {
                    strcpy(mult_players_ip[i], *(splitted_mult_info));
                    mult_players_port[i] = (int) *(splitted_mult_info + 1);
                    mult_players_qty++;
                    free(*(splitted_mult_info));
                    free(*(splitted_mult_info + 1));
                    free(splitted_mult_info);
                }

              } else if (strcmp(*(splitted_response + i), "O jogo multiplayer começou!") == 0) {
                sprintf(response_mult, "%s", *(splitted_response + i));
                flag_read_mult_info = 0;
              } else {
                sprintf(response_mult, "%s\n%s\n", response_mult, *(splitted_response + i));
              }

              free(*(splitted_response + i));
            }
            free(splitted_response);
          }

          // Aqui, mult_players_port e mult_players_ip contém os dados em ordem de porta e ip dos jogadores da partida

          printf("%s\n", response_mult);

        } else {
          printf("%s\n", response);
        }
      }

      if (flag_carrasco_waiting && strcmp(response,"\nPalavra inválida\nSó são aceitas palavras com letras minusculas, sem acentos\nQual palavra quer utilizar?\n") != 0) {
        printf("Aguardando jogadores...\n");
        fflush(stdout);
        sleep(10);
        flag_carrasco_waiting = 0;
        sprintf(message, "start_mult_game");
        write(sockfd, message, strlen(message));
        flag_carrasco_playing = 1;
        continue;
      }

      if (strcmp(response,"\nVocê é o carrasco\nQual palavra quer utilizar?\n") == 0) {
        flag_carrasco_waiting = 1;
      }

      printf("Cliente: ");
      fgets (message, MAXDATASIZE, stdin);

      if(strcmp(message, EXIT_COMMAND) == 0) {
         break;
      }

      write(sockfd, message, strlen(message));
   }
}

char** str_split(char* a_str, const char a_delim) {
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}
