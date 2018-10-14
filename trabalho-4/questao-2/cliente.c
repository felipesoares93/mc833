#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_LINE 256

int GetSocket(int family, int type, int flags);
int ProcessConnection(int s);
void BindConnection(struct sockaddr_in server, int s, char ip[MAX_LINE]);

int main(int argc, char *argv[]) {
  struct sockaddr_in server;
  char ip[MAX_LINE];
  char error[MAX_LINE + 1];
  int s;

  /* verificação de argumentos */
  if (argc < 2) {
    strcpy(error,"uso: ");
    strcat(error,argv[0]);
    strcat(error," <Address>");
    perror(error);
    exit(1);
  } else if (argc < 3) {
    strcpy(error,"uso: ");
    strcat(error,argv[0]);
    strcat(error," <Port>");
    perror(error);
    exit(1);
  }

  /* criação de socket ativo*/
  s = GetSocket(AF_INET, SOCK_STREAM, 0);

  server.sin_addr.s_addr = inet_addr(argv[1]);
  server.sin_family = AF_INET;
  server.sin_port = htons((unsigned short)strtol(argv[2],NULL,10));

  BindConnection(server, s, ip);

  /* ler e enviar linhas de texto, receber eco */
  while (1) {
      if (!ProcessConnection(s)) {
        break;
      }
  }

  close(s);
  return 0;
}


int GetSocket(int family, int type, int flags) {
  int s;
  /* criação de socket passivo */
  // Argumentos 1) Internet domain 2) Stream socket 3) Default protocol (TCP)
  s = socket(family, type, flags);
  if (s == -1) {
    printf("Socket nao foi criado");
    exit(1);
  }

  puts("Socket criado");
  return s;
}


void BindConnection(struct sockaddr_in server, int s, char ip[MAX_LINE]) {
  /* estabelecimento da conexão */
  if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("Falha no estabelecimento da conexao");
    exit(1);
  }

  inet_ntop(AF_INET, &(server.sin_addr), ip, INET_ADDRSTRLEN);
  printf("Conectado em\nIP: %s Porta: %u\n", ip, ntohs(server.sin_port));
}


int ProcessConnection(int s) {
  char buf[MAX_LINE];

  memset(buf, 0, MAX_LINE);
  //Recebe a resposta do servidor
  if (recv(s, buf, MAX_LINE, 0) < 0) {
    puts("Recebimento falhou");
    return 0;
  }

  if (strlen(buf) == 0 || strcmp(buf, "") == 0) {
    return 1;
  }

  int systemResp = system(buf);

  // 32512 é o valor retornado quando a funcão nao roda o metodo desejado
  if (systemResp < 0 || systemResp == 32512) {
    // Erro: comando nao existe
    memset(buf, 0, MAX_LINE);
    strcpy(buf, "Comando inválido");
  }

  //Envia a mensagem
  if (send(s, buf, strlen(buf), 0) < 0) {
    puts("Envio falhou");
    return 0;
  }

  return 1;
}
