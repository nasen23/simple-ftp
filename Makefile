CC = gcc
DEBUGGER = gdb
CFLAGS = -g
COMMON = common/common.h
DIRCLIENT = ./client
DIRSERVER = ./server
SRCCLIENT = ${COMMON} $(wildcard ${DIRCLIENT}/*.c) $(wildcard ${DIRCLIENT}/*.h)
SRCSERVER = ${COMMON} $(wildcard ${DIRSERVER}/*.c) $(wildcard ${DIRSERVER}/*.h)
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
