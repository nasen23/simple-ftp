CC = gcc
CFLAGS = -g
SRCCLIENT = client.c
SRCSERVER = server.c
BIN = bin
BINCLIENT = ${BIN}/client.out
BINSERVER = ${BIN}/server.out

all: clean client server

client: ${SRCCLIENT}
		${CC} ${CFLAGS} $^ -o ${BINCLIENT}

server: ${SRCSERVER}
		${CC} ${CFLAGS} $^ -o ${BINSERVER}


clean:
		rm -f ${BINCLIENT}
		rm -f ${BINSERVER}
