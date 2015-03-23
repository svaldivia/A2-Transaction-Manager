#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>

#include "common.h"
#include "msg.h"
#include "tmanager.h"
#include "server.h"

vclock_t tm_clock;
txmanager_t txmanager;


int main(int argc, char ** argv) 
{
    char logFileName[128];
    int  logfileFD;
    int  port;

    if (argc != 2) {
        printf("usage: %s portNum\n", argv[0]);
        return -1;
    }

    /* Port & log file name */
    port = atoi(argv[1]);
    snprintf(logFileName, sizeof(logFileName), "TmanagerLog_%d.log", port);

    printf("Port number:              %d\n", port);
    printf("Log file name:            %s\n", logFileName);

    /* Init vector clock */
    vclock_init(&tm_clock);
    
    /* */
    txmanager.server = NULL;

    /* Set up server */
    server_alloc(&txmanager.server, port, 10);
    server_listen(txmanager.server);

    struct sockaddr_in recv_addr;
    
    message_t msg;
    while(1) {
        server_recv(txmanager.server, &msg, &recv_addr);

        /* Update vector clock */
        vclock_update(port, &tm_clock, msg.vclock);

        /* Handle message */
        switch(msg.type) {
            case BEGINTX:
                printf("Begining a transaction");
                addTransaction(msg.tid);
                break;
            case COMMIT:
                printf("Commit request");
                break;
            case ABORT:
                printf("Aborting transaction");
                break;
            case PREPARED:
                printf("Node is prepared");
                break;
        }
    }

    return 0;
}

/* */
transaction_t* addTransaction(uint32_t tid){
    int i;
    for (i = 0; i < MAX_TRANSACTIONS; i++){
        /* tid exists */
        if (txmanager.transactions[i].tid == tid){
            /* send error */
           // server_send(txmanager->server, wstate.tm_host, wstate.tm_port, &msg);
        }
    }


/* tid exist */
/*  */
/* */
    return NULL;
}





