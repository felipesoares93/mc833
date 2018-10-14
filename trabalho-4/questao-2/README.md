# Trabalho 4 - Questão 2

## Getting Started

Para execução dos programas, incialmente deve-se compilar or arquivos cliente.c e servidor.c através do comando make:

```
$ make
```

Para que o programa cliente possa se comunicar com o servidor, a porta escolhida deve estar abilitada e disponível na máquina servidor.

## Running

Para executar o compilado servidor, rodar o seguinte comando em um terminal:

```
$ ./servidor [PORTA]
```

onde [PORTA] é a porta utilizada pelo servidor para expor a conexão.

Para executar o compilado cliente, rodar o seguinte comando em um outro terminal:

```
$ ./cliente [IP] [PORTA]
```

onde [IP] é o endereço de IP do servidor e [PORTA] a porta de conexão do servidor.

## Example

Executando ambos programas em uma mesma máquina A mas em diferentes terminais:

```
$ ./servidor 8000
```

```
$ ./cliente 127.0.0.1 8000
```
