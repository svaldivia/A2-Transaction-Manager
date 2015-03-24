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
        /* Increment clock */
        vclock_increment(txmanager.port,&tm_clock);
        
        /* Handle message */
        switch(msg.type) {
            case BEGINTX:
                printf("Begining a transaction");
                addTransaction(msg.tid,&recv_addr);
                break;
            case COMMIT: {
                printf("Commit tid:%d request",msg.tid);
                transaction_t* transaction = findTransaction(msg.tid);
                if (transaction == NULL){
                    printf("Transaction was not found");
                } else {
                    /* Prepare to commit*/
                    message_t msg;
                    message_init(&msg,&tm_clock);
                    msg.type = PREPARE_TO_COMMIT;
                    /* Send to workers in transaction */ 
                    sendToAllWorkers(transaction, &msg);
                }
                break;
                }
            case ABORT:{
                printf("Aborting transaction tid:%d",msg.tid);
                
                transaction_t* transaction = findTransaction(msg.tid);
                if (transaction == NULL){
                    printf("Transaction was not found");
                } else {
                     /* Abort transaction*/
                    message_t msg;
                    message_init(&msg,&tm_clock);
                    msg.type = ABORT;
                
                    /* Send to workers in transaction */ 
                    sendToAllWorkers(transaction, &msg);
                }
                break;
                }
            case PREPARED:
                printf("Node is prepared");
                break;
            case JOINTX:
                break;
        }
    }

    return 0;
}

/* */
transaction_t* addTransaction(uint32_t tid, struct sockaddr_in* dest_addr){
    int i;
    for (i = 0; i < MAX_TRANSACTIONS; i++){
        transaction_t* transaction = &txmanager.transactions[i];
        /* tid exists */
        if (transaction->tid == tid){
            /* Create error message*/
            message_t msg;
            message_init(&msg,&tm_clock);
            msg.type = TX_ERROR;
            msg.value = 1;
            strcpy(msg.strdata,"TID already exists");

            /* send error */
            server_send(txmanager.server,dest_addr, &msg);

        } else if(transaction->state == COMMIT_STATE || transaction->state == ABORT_STATE || transaction->tid == 0) {
            
            /* Reset Transaction */
            memset(transaction,0,sizeof(transaction_t));

            /* Insert transaction */
            transaction->state = BEGIN_STATE;
            transaction->tid = tid;
            transaction->nodeCount = 1;
            transaction->nodes[0].nid = ntohs(dest_addr->sin_port);
            memcpy(&transaction->nodes[0].address, dest_addr,sizeof (transaction_t));
            
            printf("Transaction:\n tid: %d state: %d node:%d \n was added successfully",transaction->tid, transaction->state,transaction->nodes[0].nid);
            
            return transaction;
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
    
    server_send(txmanager.server,dest_addr, &msg);
    return NULL;
}

/* Find transaction by TID */
transaction_t* findTransaction(uint32_t tid){
    int i;
    for (i = 0; i < MAX_TRANSACTIONS; i++){
        /* tid exists */
        if (txmanager.transactions[i].tid == tid){
            return &txmanager.transactions[i];
        }
    }
    return NULL;
}

/* Send to all nodes in transaction */
void sendToAllWorkers(transaction_t* transaction, message_t* msg){
    int i;
    for(i = 0; i<MAX_NODES; i++){
        
        worker_t* worker = &transaction->nodes[i];
        
        /* Send message to worker */
        if(worker->nid !=0){
            printf("Sending %d message to %d\n",msg->type, worker->nid);
            server_send(txmanager.server,&worker->address, msg);
        }
    }
}

