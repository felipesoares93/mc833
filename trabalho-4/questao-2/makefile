CC=gcc
FLAGS = -Wall

all : cliente servidor

cliente : cliente.c
	$(CC) $(FLAGS) -o cliente cliente.c
servidor : servidor.c
	$(CC) $(FLAGS) -o servidor servidor.c

clean :
	@rm cliente servidor 