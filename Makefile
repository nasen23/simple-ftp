CC = gcc
DEBUGGER = gdb
CFLAGS = -g
SRCCLIENT = common.h client.c
SRCSERVER = common.h server.h server.c
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

runserver:
		./${BINSERVER}

runclient:
		./${BINCLIENT}

debugserver:
		${DEBUGGER} ./${BINSERVER}

debugclient:
		${DEBUGGER} ./${BINCLIENT}
