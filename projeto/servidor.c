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
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include "basic.h"
#include "socket_helper.h"

#define LISTENQ 10
#define MAXDATASIZE 4096
#define EXIT_COMMAND "exit\n"
#define CLI_MSG_SIZE 100

volatile sig_atomic_t keepRunning = 1;
int carrasco_palavra;
char *carrasco_palavra_ptr;
int carrasco_flag;
int *carrasco_ptr;
int jogadores_online;
int *jogadores_online_ptr;

void sighandler(int dummy) {
  keepRunning = 0;
  shmdt((void *) carrasco_palavra_ptr);
  shmdt((void *) carrasco_ptr);
  shmdt((void *) jogadores_online_ptr);
  shmctl(carrasco_palavra, IPC_RMID, NULL);
  shmctl(carrasco_flag, IPC_RMID, NULL);
  shmctl(jogadores_online, IPC_RMID, NULL);
  exit(0);
}

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

   signal(SIGINT, sighandler);
   signal(SIGTSTP, sighandler);

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

   //////////////////////////////////////////////////

   // VARIAVEIS DE MEMORIA COMPARTILHADA

   /////////////////////////////////////////////////
   carrasco_flag = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
   if (carrasco_flag < 0) {
      printf("*** shmget error (server) ***\n");
      exit(1);
   }

   carrasco_ptr = (int *) shmat(carrasco_flag, NULL, 0);
   if ((int) carrasco_ptr == -1) {
      printf("*** shmat error (server) ***\n");
      exit(1);
   }
   carrasco_ptr[0] = 0;

   int carrasco_palavra;
   char *carrasco_palavra_ptr;

   carrasco_palavra = shmget(IPC_PRIVATE, (MAXDATASIZE+1)*sizeof(char), IPC_CREAT | 0666);
   if (carrasco_palavra < 0) {
      printf("*** shmget error (server) ***\n");
      exit(1);
   }

   carrasco_palavra_ptr = (char *) shmat(carrasco_palavra, NULL, 0);
   if ((char) carrasco_palavra_ptr == -1) {
      printf("*** shmat error (server) ***\n");
      exit(1);
   }

   jogadores_online = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
   if (jogadores_online < 0) {
      printf("*** shmget error (server) ***\n");
      exit(1);
   }

   jogadores_online_ptr = (int *) shmat(jogadores_online, NULL, 0);
   if ((int) jogadores_online_ptr == -1) {
      printf("*** shmat error (server) ***\n");
      exit(1);
   }
   jogadores_online_ptr[0] = 0;

   ///////////////////////////////////////////////////
   ///////////////////////////////////////////////////

   // Configuração dos sockets
   port = atoi(argv[1]);

   listenfd = Socket(AF_INET, SOCK_STREAM, 0);

   servaddr = ServerSockaddrIn(AF_INET, INADDR_ANY, port);

   Bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

   Listen(listenfd, LISTENQ);

   while (keepRunning) {
      pid_t pid;
      int n;
      struct sockaddr_in clientaddr;
      socklen_t clientaddr_len = sizeof(clientaddr);

      connfd = Accept(listenfd, (struct sockaddr *) &clientaddr, &clientaddr_len);

      if((pid = fork()) == 0) {
         Close(listenfd);

         char *palavra_para_acertar;
         char palpite[MAXDATASIZE+1],msgresp[MAXDATASIZE+1],letras_dispo[MAXDATASIZE+1],
         resposta_ao_cliente[MAXDATASIZE+1];
         int menu,vidas,r, i, c, jogo_perdido, letra_encontrada_na_palavra, resp_cliente_invalida,
         jogo_vencido, resp_cliente_letra, resp_cliente_palavra, letra_ja_utilizada, palpite_palavra_errado,
         jogo_finalizado,jogando, letras_na_palavra_para_acertar, carrasco_cliente_enviando_palavra,
         carrasco_cliente_aguardando_start, carrasco_cliente,palavra_carrasco_invalida,online;
         int *palavras_utilizadas;

         menu = 1;
         jogo_finalizado = 0;
         jogando = 0;
         carrasco_cliente_enviando_palavra = 0;
         carrasco_cliente_aguardando_start = 0;
         carrasco_cliente = 0;
         online = 0;
         palavras_utilizadas = (int *) malloc(nwords*sizeof(int));
         i = 0;
         while(i < nwords) {
            palavras_utilizadas[i] = 0;
            i++;
         }
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

            if (strcmp(msgresp, "-h") == 0) {
               sprintf(resposta_ao_cliente, "\nRegras do jogo\n\n- O carrasco escolhe uma palavra sem mostrar ao enforcado\n- Informa ao enforcado quantas letras tem a palavra\n- O enforcado então diz uma letra do alafabeto\n- O carrasco verifica se esta letra esta contida na palavra\n    - Se estiver, o mesmo preenche os espaços em branco correspondentes à letra\n    - Caso não estiver, o carrasco tira uma vida do enforcado\n- Se o enforcado perde todas as vidas, perde o jogo\n- Se o enforcado fizer a tentativa de  adivinhar a palavra inteira e errar, perde o jogo\n\nDesenvolvedores:\nArthur Maia Mendes\nFelipe Carvalho\n");
               write(connfd, resposta_ao_cliente, strlen(resposta_ao_cliente));
               continue;
            }

            if (carrasco_cliente_enviando_palavra == 1) {

               i = 0;
               palavra_carrasco_invalida = 0;
               while (i < strlen(msgresp)) {
                  if ((msgresp[i] < 97) ||(msgresp[i] > 122)) {
                     palavra_carrasco_invalida = 1;
                  }
                  i++;
               }

               if (palavra_carrasco_invalida == 1) {
                  sprintf(resposta_ao_cliente, "\nPalavra inválida\nSó são aceitas palavras com letras minusculas, sem acentos\nQual palavra quer utilizar?\n");
                  write(connfd, resposta_ao_cliente, strlen(resposta_ao_cliente));
                  continue;
               }

               carrasco_cliente_enviando_palavra = 0;
               sprintf(carrasco_palavra_ptr, "%s", msgresp);
               printf("%s", carrasco_palavra_ptr);
            }

            if (carrasco_cliente == 1) {
               if (strcmp(msgresp, "sair") == 0) {
                  carrasco_ptr[0] = 0;
                  jogadores_online_ptr[0]--;
                  sprintf(carrasco_palavra_ptr, "");
                  Close(connfd);
                  exit(0);
                } else if (strcmp(msgresp, "start_mult_game") == 0) {
                  carrasco_cliente_aguardando_start = 0;
                  if (jogadores_online_ptr[0] == 0) {
                    carrasco_ptr[0] = 0;
                    menu = 1;
                    sprintf(carrasco_palavra_ptr, "");
                    sprintf(resposta_ao_cliente, "\nO jogo não pode iniciar pois não há jogadores online!\n\n1) Iniciar partida simples\n2) Ser carrasco ao iniciar partida\n3) Jogar no modo multiplayer\n");
                    write(connfd, resposta_ao_cliente, strlen(resposta_ao_cliente));
                    continue;
                  } else {
                    sprintf(resposta_ao_cliente, "\nO jogo multiplayer começou!\nEsta partida será disputada por %d jogadores\nDigite \"sair\" para finalizar o jogo\n", jogadores_online_ptr[0]);
                    write(connfd, resposta_ao_cliente, strlen(resposta_ao_cliente));
                    continue;
                  }
                } else {
                  if (!carrasco_cliente_aguardando_start) {
                    sprintf(resposta_ao_cliente, "\nDigite \"sair\" para finalizar o jogo\n");
                  } else {
                    sprintf(resposta_ao_cliente, "waiting_mult_game");
                  }

                  write(connfd, resposta_ao_cliente, strlen(resposta_ao_cliente));
                  continue;
               }
            }

            if (jogo_finalizado == 1) {
               if (online == 1) {
                  online = 0;
                  jogadores_online_ptr[0]--;
               }

               if (strcmp(msgresp, "sim") == 0) {
                  jogo_finalizado = 0;
                  menu = 1;
                  sprintf(resposta_ao_cliente, "\nBem vindo ao jogo da forca\n____\n\n1) Iniciar partida simples\n2) Ser carrasco ao iniciar partida\n3) Jogar no modo multiplayer\n");
                  write(connfd, resposta_ao_cliente, strlen(resposta_ao_cliente));
                  continue;
               } else if (strcmp(msgresp, "não") == 0) {
                  Close(connfd);
                  exit(0);
               } else {
                  sprintf(resposta_ao_cliente, "\nResposta inválida\n");
                  write(connfd, resposta_ao_cliente, strlen(resposta_ao_cliente));
                  continue;
               }

            }

            if (menu == 1) {

               // Inicialização partida simples
               if (strcmp(msgresp, "1") == 0) {

                  menu = 0;
                  jogando = 1;
                  vidas = 6;
                  sprintf(letras_dispo, "| a | b | c | d | e | f | g | h | i | j | k | l | m | n | o | p | q | r | s | t | u | v | x | y | z |");

                  // Escolha da palavra para cliente
                  i = 0;
                  r = rand() % nwords;
                  while ((i < nwords) && (palavras_utilizadas[r] == 1)) {
                     r = rand() % nwords;
                     i++;
                  }
                  if (i == nwords) {
                     i = 0;
                     while (i < nwords) {
                        palavras_utilizadas[i] = 0;
                        i++;
                     }
                     r = rand() % nwords;
                  }
                  palavras_utilizadas[r] = 1;

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

                  if (carrasco_ptr[0] == 0) {
                    sprintf(resposta_ao_cliente, "\nVocê é o carrasco\nQual palavra quer utilizar?\n");
                    carrasco_ptr[0] = 1;
                    write(connfd, resposta_ao_cliente, strlen(resposta_ao_cliente));
                    carrasco_cliente_enviando_palavra = 1;
                    carrasco_cliente = 1;
                    carrasco_cliente_aguardando_start = 1;
                    continue;
                  } else {
                     sprintf(resposta_ao_cliente, "\nJá tem um carrasco. Escolha outra opção.\n\n1) Iniciar partida simples\n2) Ser carrasco ao iniciar partida\n3) Jogar no modo multiplayer\n");
                     write(connfd, resposta_ao_cliente, strlen(resposta_ao_cliente));
                     menu = 1;
                     continue;
                  }

               } else if (strcmp(msgresp, "3") == 0) {
                  if (carrasco_ptr[0] == 0) {
                     sprintf(resposta_ao_cliente, "\nNão há carrasco.\nJogo indisponível\nEscolha outra opção\n\n1) Iniciar partida simples\n2) Ser carrasco ao iniciar partida\n3) Jogar no modo multiplayer\n");
                     write(connfd, resposta_ao_cliente, strlen(resposta_ao_cliente));
                     menu = 1;
                     continue;
                  } else {
                     online = 1;
                     jogadores_online_ptr[0]++;
                     menu = 0;
                     jogando = 1;
                     vidas = 3;
                     sprintf(letras_dispo, "| a | b | c | d | e | f | g | h | i | j | k | l | m | n | o | p | q | r | s | t | u | v | x | y | z |");
                     palavra_para_acertar = (char *)malloc((MAXDATASIZE+1)*sizeof(char));
                     i = 0;
                     while(i < MAXDATASIZE+1) {
                        palavra_para_acertar[i] = carrasco_palavra_ptr[i];
                        i++;
                     }
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
                     sprintf(resposta_ao_cliente, "\nEssa partida será disputada por %d jogadores\nA partida de jogo da forca começou!\n_____\n\nVocê possui %d vidas\nA palavra possui %lu caracteres\n\n%s\n", jogadores_online_ptr[0], vidas, strlen(palavra_para_acertar), palpite);

                     write(connfd, resposta_ao_cliente, strlen(resposta_ao_cliente));
                     continue;
                  }
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
                     sprintf(resposta_ao_cliente, "\nLetra inválida\nVocê perdeu o jogo\nSe deseja ir ao menu digite \"sim\"\nSe quiser sair do programa, digite \"não\"\n");
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

                  printf("<%s-%d> palpite: %s\n", inet_ntoa(clientaddr.sin_addr),(int) ntohs(clientaddr.sin_port), msgresp);
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
                           sprintf(resposta_ao_cliente, "\nA palavra não tem nenhuma letra \'%s\'\nVocê perdeu o jogo\nSe deseja ir ao menu digite \"sim\"\nSe quiser sair do programa, digite \"não\"\n", msgresp);
                           jogando = 0;
                           jogo_finalizado = 1;
                        } else {
                           sprintf(resposta_ao_cliente, "\nA palavra não tem nenhuma letra \'%s\'\n%s\nVocê agora possui %d vidas\n", msgresp, letras_dispo, vidas);
                        }
                     } else {
                        if (letras_na_palavra_para_acertar > 0) {
                           sprintf(resposta_ao_cliente, "\nPalavra: %s\n%s\nVidas: %d\n", palpite, letras_dispo, vidas);
                        } else {
                           sprintf(resposta_ao_cliente, "\nVocê adivinhou a palavra \"%s\"! Parabéns!\nSe deseja ir ao menu digite \"sim\"\nSe quiser sair do programa, digite \"não\"\n", palavra_para_acertar);
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
                     sprintf(resposta_ao_cliente, "\nVocê adivinhou a palavra!\nSe deseja ir ao menu digite \"sim\"\nSe quiser sair do programa, digite \"não\"\n");

                  } else {
                     sprintf(resposta_ao_cliente, "\nA palavra correta era \"%s\", você perdeu!\nSe deseja ir ao menu digite \"sim\"\nSe quiser sair do programa, digite \"não\"\n", palavra_para_acertar);
                  }
                  jogo_finalizado = 1;
                  jogando = 0;
                  write(connfd, resposta_ao_cliente, strlen(resposta_ao_cliente));
                  continue;
               }
            }
         }

         // Verifica se o cliente que está desconectando é o carrasco
         if (carrasco_cliente == 1) {
           carrasco_ptr[0] = 0;
           carrasco_cliente_aguardando_start = 0;
         }
      }
      Close(connfd);
   }

   printf("Server has detected the completion of its child...\n");
   shmdt((void *) carrasco_palavra_ptr);
   shmdt((void *) carrasco_ptr);
   shmdt((void *) jogadores_online_ptr);
   printf("Server has detached its shared memory...\n");
   shmctl(carrasco_palavra, IPC_RMID, NULL);
   shmctl(carrasco_flag, IPC_RMID, NULL);
   shmctl(jogadores_online, IPC_RMID, NULL);
   printf("Server has removed its shared memory...\n");
   printf("Server exits...\n");
   exit(0);

   return(0);
}
