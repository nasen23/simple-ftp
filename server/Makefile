CC = gcc
CFLAGS = -g -Wall
SRCSERVER = $(wildcard *.c) $(wildcard *.h)
BINSERVER = server

all: clean build

build:
	${CC} ${CFLAGS} ${SRCSERVER} -o ${BINSERVER}

clean:
	rm -rf server
