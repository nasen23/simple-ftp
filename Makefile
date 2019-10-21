CC = gcc
DEBUGGER = gdb
CFLAGS = -g -Wall
COMMON = common/common.h
DIRCLIENT = ./client
DIRSERVER = ./server
SRCCLIENT = ${COMMON} $(wildcard ${DIRCLIENT}/*.c) $(wildcard ${DIRCLIENT}/*.h)
SRCSERVER = ${COMMON} $(wildcard ${DIRSERVER}/*.c) $(wildcard ${DIRSERVER}/*.h)
BINCLIENT = client/client
BINSERVER = server/server

all: clean server

client: ${SRCCLIENT}
		${CC} ${CFLAGS} $^ -o ${BINCLIENT}

server: ${SRCSERVER}
		${CC} ${CFLAGS} $^ -o ${BINSERVER}

clean:
		rm -f ${BINSERVER}

runserver:
		./${BINSERVER}

runclient:
		./${BINCLIENT}

debugserver:
		${DEBUGGER} ./${BINSERVER}

debugclient:
		${DEBUGGER} ./${BINCLIENT}
