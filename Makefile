all: tmanager tworker cmd dumpObject txlog_dump

CLIBS=-pthread
CC=gcc
CPPFLAGS=
CFLAGS=-g -Werror-implicit-function-declaration -pedantic -std=gnu99

tworker: tworker.h msg.c tworker.c server.c common.c tx_worker_thread.c shitviz.c txlog.c objstore.c
	$(CC) $(CFLAGS) $(CLIBS) -o tworker msg.c tworker.c server.c common.c tx_worker_thread.c shitviz.c txlog.c objstore.c

tmanager: tmanager.c msg.c common.c server.c shitviz.c txlog.c
	$(CC) $(CFLAGS) -o tmanager tmanager.c common.c server.c msg.c shitviz.c txlog.c

dumpObject: dumpObject.c tworker.h
	$(CC) $(CFLAGS) -o dumpObject dumpObject.c

txlog_dump: txlog_dump.c txlog.c common.c
	$(CC) $(CFLAGS) -o txlog_dump txlog_dump.c txlog.c common.c

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


