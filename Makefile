all: tmanager tworker cmd dumpObject


CLIBS=-pthread
CC=gcc
CPPFLAGS=
CFLAGS=-g -Werror-implicit-function-declaration -pedantic -std=c99

tworker: tworker.h msg.c tworker.c server.c common.c
	$(CC) $(CFLAGS) -o tworker msg.c tworker.c server.c common.c

tmanager: tmanager.c msg.c common.c server.c
	$(CC) $(CFLAGS) -o tmanager tmanager.c common.c server.c msg.c

dumpObject: dumpObject.c tworker.h
	$(CC) $(CFLAGS) -o dumpObject dumpObject.c

cmd: command.c common.c msg.c
	$(CC) $(CFLAGS) -o cmd command.c common.c msg.c

server: server.c msg.c common.c
	$(CC) $(CFLAGS) -o server server.c msg.c common.c

cleanlogs:
	rm -f *.log

cleanobjs:
	rm -f *.data

clean:
	rm -f *.o
	rm -f tmanager tworker cmd dumpObject

scrub: cleanlogs cleanobjs clean


