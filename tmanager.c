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
#include <string.h>

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
    txmanager.port = port;

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
                addTransaction(msg.tid,&recv_addr);
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
transaction_t* addTransaction(uint32_t tid, struct sockaddr_in* dest_addr){
    int i;
    for (i = 0; i < MAX_TRANSACTIONS; i++){
        /* tid exists */
        if (txmanager.transactions[i].tid == tid){
            /* Create error message*/
            message_t msg;
            message_init(&msg,&tm_clock);
            msg.type = TX_ERROR;
            msg.value = 1;
            strcpy(msg.strdata,"TID already exists");

            /* send error */
            vclock_increment(txmanager.port,&tm_clock);
            server_send(txmanager.server,dest_addr, &msg);
        } else if(txmanager.transactions[i].state == COMMIT_STATE || txmanager.transactions[i].state == ABORT_STATE || txmanager.transactions[i].tid == 0) {
            /* Insert transaction */
            txmanager.transactions[i].state = BEGIN_STATE;
            txmanager.transactions[i].tid = tid;
            txmanager.transactions[i].nodeCount = 1;
            txmanager.transactions[i].nodes[0] = ntohs(dest_addr->sin_port);
            printf("Transaction:\n tid: %d state: %d node:%d \n was added successfully",txmanager.transactions[i].tid, txmanager.transactions[i].state,txmanager.transactions[i].nodes[0]);
            return &txmanager.transactions[i];
        }
    }
    /* Transaction log full */
    printf("No more space in transaction manager");
    
    /* Create error message*/
    message_t msg;
    message_init(&msg,&tm_clock);
    msg.type = TX_ERROR;
    msg.value = 1;
    strcpy(msg.strdata,"Transaction queue full");
    
    vclock_increment(txmanager.port,&tm_clock);
    server_send(txmanager.server,dest_addr, &msg);

    return NULL;
}





