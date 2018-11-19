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
#include <ctype.h>

#include "basic.h"
#include "socket_helper.h"

#define LISTENQ 10              
#define MAXDATASIZE 4096         
#define EXIT_COMMAND "exit\n"
#define CLI_MSG_SIZE 100

int main (int argc, char **argv) {
   
   int    listenfd,              
          connfd,               
          port;                  
   struct sockaddr_in servaddr;  
   char   error[MAXDATASIZE + 1];     

   if (argc != 2) {
      strcpy(error,"uso: ");
      strcat(error,argv[0]);
      strcat(error," <Port>");
      perror(error);
      exit(1);
   }

   // Leitura das palavras
   char wordsfile[30];
   sprintf(wordsfile, "palavras.txt"); 
   int nwords = GetNLines(wordsfile);
   char words[nwords][40];
   int i;

   FILE *f = fopen(wordsfile, "r");
   
   for (i = 0; i < nwords; i++) {
      if (fgets(words[i], sizeof(words[i]), f) != NULL) {
         words[i][strlen(words[i])-1] = '\0';
      }
   }

   fclose(f);

   // Configuração dos sockets
   port = atoi(argv[1]);

   listenfd = Socket(AF_INET, SOCK_STREAM, 0);

   servaddr = ServerSockaddrIn(AF_INET, INADDR_ANY, port);

   Bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

   Listen(listenfd, LISTENQ);

   for ( ; ; ) {
      pid_t pid;
      int n;

      struct sockaddr_in clientaddr;
      socklen_t clientaddr_len = sizeof(clientaddr);

      connfd = Accept(listenfd, (struct sockaddr *) &clientaddr, &clientaddr_len);

      if((pid = fork()) == 0) {
         Close(listenfd);


         int menu;
         char *palavra_para_acertar;
         char palpite[MAXDATASIZE+1];
         char msgresp[MAXDATASIZE+1];
         char letras_dispo[MAXDATASIZE+1];
         char resposta_ao_cliente[MAXDATASIZE+1];
         int vidas;
         int r;
         int i;
         int c;
         int jogo_perdido;
         int letra_encontrada_na_palavra;
         int resp_cliente_invalida;
         int jogo_vencido;
         int resp_cliente_letra;
         int resp_cliente_palavra;
         int letra_ja_utilizada;
         int palpite_palavra_errado;
         int jogo_finalizado;
         int jogando;
         int primeira_conexao;
         int letras_na_palavra_para_acertar;
         int carrasco;
         char carrasco_arq[MAXDATASIZE+1];
         char jogadores_arq[MAXDATASIZE+1];

         primeira_conexao = 1;
         menu = 1;
         jogo_finalizado = 0;
         jogando = 0;
         sprintf(carrasco_arq, "carrasco.txt");
         sprintf(jogadores_arq, "jogadores.txt");
         srand(time(NULL));
         sprintf(resposta_ao_cliente, "\nBem vindo ao jogo da forca\n____\n\n1) Iniciar partida simples\n2) Ser carrasco ao iniciar partida\n3) Jogar no modo multiplayer\n");

         write(connfd, resposta_ao_cliente, strlen(resposta_ao_cliente));

         if (getpeername(connfd, (struct sockaddr *) &clientaddr, &clientaddr_len) == -1) {
            perror("getpeername() failed");
            return(0);
         }

         printf("<%s-%d> conectado\n", inet_ntoa(clientaddr.sin_addr),(int) ntohs(clientaddr.sin_port));

         while ((n = read(connfd, msgresp, MAXDATASIZE)) > 0) {

            i = 0;
            while ((msgresp[i] != '\n') && (msgresp[i] != '\0')) {
               i++;
            }
            while (i < MAXDATASIZE) {
               msgresp[i] = '\0';
               i++;
            }

            printf("<%s-%d>: %s\n", inet_ntoa(clientaddr.sin_addr),(int) ntohs(clientaddr.sin_port), msgresp);

            



            if (jogo_finalizado == 1) {

               if (strcmp(msgresp, "sim") == 0) {
                  jogo_finalizado = 0;
                  menu = 1;
                  sprintf(resposta_ao_cliente, "\nBem vindo ao jogo da forca\n____\n\n1) Iniciar partida simples\n2) Ser carrasco ao iniciar partida\n3) Jogar no modo multiplayer\n");
                  write(connfd, resposta_ao_cliente, strlen(resposta_ao_cliente));
                  continue;
               } else if (strcmp(msgresp, "não") == 0) {
                  sprintf(resposta_ao_cliente, "\nObrigado por jogar!\n");
                  write(connfd, resposta_ao_cliente, strlen(resposta_ao_cliente));
                  break;
               } else {
                  sprintf(resposta_ao_cliente, "\nResposta inválida\n");
                  write(connfd, resposta_ao_cliente, strlen(resposta_ao_cliente));
                  continue;
               }

            }













            if (menu == 1) {
               menu = 0;

               // Inicialização partida simples
               if (strcmp(msgresp, "1") == 0) {

                  jogando = 1;
                  vidas = 6;
                  sprintf(letras_dispo, "| a | b | c | d | e | f | g | h | i | j | k | l | m | n | o | p | q | r | s | t | u | v | x | y | z |");

                  // Escolha da palavra para cliente
                  r = rand() % nwords;
                  palavra_para_acertar = words[r];
                  letras_na_palavra_para_acertar = strlen(palavra_para_acertar);
                  printf("<%s-%d> palavra: %s\n", inet_ntoa(clientaddr.sin_addr),(int) ntohs(clientaddr.sin_port), palavra_para_acertar);

                  // Montagem da string de palpite
                  i = 0;
                  while (i < MAXDATASIZE) {
                     if (i >= 2*strlen(palavra_para_acertar)) {
                        palpite[i] = '\0';
                     } else {
                        if (i % 2 == 0) {
                           palpite[i] = '_';
                        } else {
                           palpite[i] = ' ';
                        }
                     }
                     i++;
                  }

                  // Mensagem de início de jogo
                  sprintf(resposta_ao_cliente, "\nA partida de jogo da forca começou!\n_____\n\nVocê possui %d vidas\nA palavra possui %lu caracteres\n\n%s\n", vidas, strlen(palavra_para_acertar), palpite);

                  write(connfd, resposta_ao_cliente, strlen(resposta_ao_cliente));
                  continue;
                  
               } else if (strcmp(msgresp, "2") == 0) {
                  printf("aqui\n");

                  carrasco = GetNLines(carrasco_arq);
                  printf("carrasco: %d", carrasco); 
                  if (carrasco == 0) {
                     f = fopen(carrasco_arq, "w");
                     fprintf(f, "<%s-%d>\n", inet_ntoa(clientaddr.sin_addr),(int) ntohs(clientaddr.sin_port));
                     fclose(f);
                  } else {

                  }


               } else if (strcmp(msgresp, "3") == 0) {

               }
            } 












            if (jogando == 1) {
               
               // Inicialização de variáveis de controle
               resp_cliente_invalida = 0;
               jogo_perdido = 0;
               jogo_vencido = 0;
               resp_cliente_letra = 0;
               resp_cliente_palavra = 0;
               letra_encontrada_na_palavra = 0;
               letra_ja_utilizada = 0;

               // Iteração com jogador

               // Jogador envia letra ou palavra para servidor
               i = 0; 
               c = 0;
               while (i < strlen(msgresp)) {
                  if (isalpha(msgresp[i])) {
                     c++;
                  }
                  i++;
               }

               if (c == 0) {
                  resp_cliente_invalida = 1;
               }


               







               if (resp_cliente_invalida == 1) {
                  vidas--;
      
                  if (vidas == 0) {
                     sprintf(resposta_ao_cliente, "\nLetra inválida\nVocê perdeu o jogo\nSe deseja jogar outra partida digite \"sim\"\nSe quiser sair do programa, digite \"não\"\n");
                     jogando = 0;
                     jogo_finalizado = 1;
                  } else {
                     sprintf(resposta_ao_cliente, "\nLetra inválida\nVocê agora possui %d vidas\n", vidas);
                  }

                  write(connfd, resposta_ao_cliente, strlen(resposta_ao_cliente));
                  continue;
               } else {

                  // Verfica se cliente enviou uma letra
                  if (c == 1) {
                     resp_cliente_letra = 1;
                  }

                  // Verifica se cliente enviou uma palavra
                  if (c > 1) {
                     resp_cliente_palavra = 1;
                  }

                  printf("<%s-%d> palpite do letra: %s\n", inet_ntoa(clientaddr.sin_addr),(int) ntohs(clientaddr.sin_port), msgresp);
               }

               




               if (resp_cliente_letra == 1) {

                  // Verifica se letra já foi utilizada
                  i = 0;
                  while ((letras_dispo[i] != msgresp[0]) && (i != strlen(letras_dispo))) {
                     i++;
                  }
                  if (i == strlen(letras_dispo)) {
                     letra_ja_utilizada = 1;
                  }

                  // Mensagem para o cliente no caso de letra já utilizada
                  if (letra_ja_utilizada == 1) {
                     sprintf(resposta_ao_cliente, "\nA letra \'%s\' já foi utilizada\n", msgresp);
                  } else {

                     // Marca que letra está sendo utilizada
                     letras_dispo[i] = ' ';

                     // Verifica se letra existe na palavra
                     i = 0;
                     letra_encontrada_na_palavra = 0;
                     while (i != strlen(palavra_para_acertar)) {
                        if (palavra_para_acertar[i] == msgresp[0]) {
                           palpite[2*i] = palavra_para_acertar[i];
                           letra_encontrada_na_palavra = 1;
                           letras_na_palavra_para_acertar--;
                        }
                        i++;
                     }

                     if (letra_encontrada_na_palavra == 0) {
                        vidas--;
                        if (vidas == 0) {
                           sprintf(resposta_ao_cliente, "\nA palavra não tem nenhuma letra \'%s\'\nVocê perdeu o jogo\nSe deseja jogar outra partida digite \"sim\"\nSe quiser sair do programa, digite \"não\"\n", msgresp);
                           jogando = 0;
                           jogo_finalizado = 1;
                        } else {
                           sprintf(resposta_ao_cliente, "\nA palavra não tem nenhuma letra \'%s\'\n%s\nVocê agora possui %d vidas\n", msgresp, letras_dispo, vidas);
                        }
                     } else {
                        if (letras_na_palavra_para_acertar > 0) {
                           sprintf(resposta_ao_cliente, "\nPalavra: %s\n%s\nVidas: %d\n", palpite, letras_dispo, vidas);
                        } else {
                           sprintf(resposta_ao_cliente, "\nVocê adivinhou a palavra \"%s\"! Parabéns!\nSe deseja jogar outra partida digite \"sim\"\nSe quiser sair do programa, digite \"não\"\n", palavra_para_acertar);
                           jogando = 0;
                           jogo_finalizado = 1;
                        }
                     }
                  }
                  write(connfd, resposta_ao_cliente, strlen(resposta_ao_cliente));
                  continue;
               }

               
               







               if (resp_cliente_palavra == 1) {
                  palpite_palavra_errado = 0;
                  for (i = 0; i < strlen(palavra_para_acertar); i++) {
                     if (palavra_para_acertar[i] != msgresp[i]) {
                        palpite_palavra_errado = 1;
                     }
                  }

                  if (palpite_palavra_errado == 0) {
                     sprintf(resposta_ao_cliente, "\nVocê adivinhou a palavra!\nSe deseja jogar outra partida digite \"sim\"\nSe quiser sair do programa, digite \"não\"\n");
                     
                  } else {
                     sprintf(resposta_ao_cliente, "\nA palavra correta era \"%s\", você perdeu!\nSe deseja jogar outra partida digite \"sim\"\nSe quiser sair do programa, digite \"não\"\n", palavra_para_acertar);
                  }
                  jogo_finalizado = 1;
                  jogando = 0;
                  write(connfd, resposta_ao_cliente, strlen(resposta_ao_cliente));
                  continue;
               } 
            }





         }

         Close(connfd);

         exit(0);
      }

      Close(connfd);
   }
   
   return(0);
}
