#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

#define MAX_PENDING 5
#define MAX_LINE 9999
#define MAX_PIDS 32

volatile int *pids;
volatile int *totalPids;

void SendCommandToClients(char command_buf[MAX_LINE], char ip[MAX_LINE], struct sockaddr_in client);
void InitSharedData();
void BindConnection(struct sockaddr_in server, int s);
void ProcessClientConnection(struct sockaddr_in client, int socketfd, char buf[MAX_LINE], char ip[MAX_LINE]);
int GetSharedSocket(int family, int type, int flags);


int main(int argc, char **argv) {
  struct sockaddr_in server, client;
  char buf[MAX_LINE], ip[MAX_LINE];
  char error[MAX_LINE + 1];
  char *address_to_connect = "127.0.0.1";
  int s, new_s, c;

  // Verifica argumentos inseridos ao executar o programa
  if (argc < 2) {
     strcpy(error,"uso: ");
     strcat(error,argv[0]);
     strcat(error," <Port>");
     perror(error);
     exit(1);
  }

  puts("Iniciando servidor...");

  // Inicializa vetores globais
  InitSharedData();

  /* criação de socket passivo */
  // Argumentos 1) Internet domain 2) Stream socket 3) Default protocol (TCP)
  s = GetSharedSocket(AF_INET, SOCK_STREAM, 0);

  /* Associar socket ao descritor */
  // Address family = Internet
  server.sin_family = AF_INET;
  // Porta na conexão
  server.sin_port = htons((unsigned short)strtol(argv[1],NULL,10));
  // Ip da conexão
  server.sin_addr.s_addr = inet_addr(address_to_connect);

  // Realiza o bind da conexao
  BindConnection(server, s);

  printf("IP local da conexão: %s \n", address_to_connect);
  printf("Porta local da conexão: %s \n", argv[1]);

  /* Criar escuta do socket para aceitar conexões */
  // Inicia conexão do socket com no máximo 5 conexoes simultaneas
  listen(s, MAX_PENDING);

  /* aguardar/aceita conexão, receber e imprimir texto na tela, enviar eco */
  puts("Aguardando conexoes...");
  c = sizeof(struct sockaddr_in);

  //aceita conexoes de um client
  while ((new_s = accept(s, (struct sockaddr *)&client, (socklen_t *)&c))) {
    // Processa a conexão com o cliente
    ProcessClientConnection(client, new_s, buf, ip);
  }

  if (new_s < 0) {
    perror("Aceite falhou");
    return 1;
  }

  close(new_s);
  close(s);

  return 0;
}


void SendCommandToClients(char command_buf[MAX_LINE], char ip[MAX_LINE], struct sockaddr_in client) {
  int i;
  printf("Enviando comando Unix [%s] para %d clientes...\n", command_buf, *totalPids);
  for (i = 0; i < *totalPids; i++) {
    printf("Enviando para socket %d...\n", pids[i]);
    //Send the message back to client
    write(pids[i], command_buf, MAX_LINE);

    if (strcmp(command_buf, "exitc") == 0 || strcmp(command_buf, "exits") == 0) {
      time_t t = time(NULL);
      struct tm tm = *localtime(&t);
      printf("Data e Hora de encerramento de conexão com o cliente IP %s, Porta %u: %d-%d-%d %d:%d:%d\n",
        ip, ntohs(client.sin_port),
        tm.tm_year + 1900, tm.tm_mon + 1,
        tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
      close(pids[i]);
    }
  }

  if (strcmp(command_buf, "exits") == 0) {
    kill(0, SIGTERM);
  }
}


void InitSharedData() {
  // Map space para o array compartilhado
  pids = mmap(0, MAX_PIDS*sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (!pids) {
    perror("mmap failed");
    exit(1);
  }
  memset((void *)pids, 0, MAX_PIDS*sizeof(int));

  totalPids = mmap(NULL, sizeof *totalPids, PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  *totalPids = 0;
}


int GetSharedSocket(int family, int type, int flags) {
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


void BindConnection(struct sockaddr_in server, int s) {
  // Bind da conexão com o socket
  if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("Bind falhou");
    exit(1);
  }
  puts("Bind completo");
}


void ProcessClientConnection(struct sockaddr_in client, int socketfd, char buf[MAX_LINE], char ip[MAX_LINE]) {
  int read_size, pid, pid2, localPidQty;
  char command_buf[MAX_LINE];

  // traduz porta e ip
  inet_ntop(AF_INET, &(client.sin_addr), ip, INET_ADDRSTRLEN);

  //TODO: call log_client_info(ip, ntohs(client.sin_port), tempo, string)

  printf("Novo cliente conectado\nIP: %s, Porta: %u\n", ip, ntohs(client.sin_port));

  pids[*totalPids] = socketfd;
  *totalPids += 1;

  // cria fork do processo
  if ((pid = fork()) == -1) {
    perror("Erro ao criar processo do fork");
    close(socketfd);
    return;
  }

  // processo pai
  if (pid > 0) {

    if ((pid2 = fork()) == -1) {
      perror("Erro ao criar processo do fork");
    }

    if (pid2 <= 0) {

      localPidQty = *totalPids;

      while (localPidQty == *totalPids) {
        //clear buffer
        bzero(command_buf, sizeof(command_buf));
        printf("Digite uma mensagem para ser enviada ao cliente: \n");
        fgets(command_buf, MAX_LINE, stdin);
        strtok(command_buf, "\n");
        SendCommandToClients(command_buf, ip, client);
      }
    }

    return;
  }

  while(1) {
    memset(buf ,0 , MAX_LINE);
    while ((read_size = recv(socketfd, buf, MAX_LINE, 0)) > 0) {
      inet_ntop(AF_INET, &(client.sin_addr), ip, INET_ADDRSTRLEN);
      printf("Nova mensagem recebida de\nIP: %s, Porta: %u\nMensagem: %s\n", ip, ntohs(client.sin_port), buf);
      memset(buf ,0 , MAX_LINE);
    }
  }

  if (read_size == 0) {
    puts("Cliente desconectado");
    fflush(stdout);
  } else if (read_size == -1) {
    perror("Recebimento falhou");
  }

  // fecha processo filho
  close(socketfd);
}
