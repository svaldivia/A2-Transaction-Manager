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
#include "txlog.h"
#include "shitviz.h"

txmanager_t txmanager = {0};


int main(int argc, char ** argv) 
{
    char txlogName[128];
    int  logfileFD;
    int  port;

    if (argc != 2) {
        printf("usage: %s portNum\n", argv[0]);
        return -1;
    }

    port = atoi(argv[1]);

    printf("Port number:              %d\n", port);
    printf("Log file name:            %s\n", txlogName);

    /* Init vector clock */
    vclock_init(txmanager.vclock);
    
    /* Set up sever */
    txmanager.server = NULL;
    txmanager.port = port;

    /* Check or Open log*/
    sprintf(txlogName, "txlog_%d.data", txmanager.port);
    txlog_open(&txmanager.txlog, txlogName); 
    
    /* Set up server */
    server_alloc(&txmanager.server, port, 10);
    server_listen(txmanager.server);

    /* Get the vector clock + set local clock */
    txlog_read_clock(txmanager.txlog, txmanager.vclock);
    if (!vclock_has(txmanager.vclock, txmanager.port)) {
        vclock_add(txmanager.vclock, txmanager.port); 
    }

    shitviz_append(port, "Started manager", txmanager.vclock);
   
    struct sockaddr_in recv_addr;
    message_t msg;
    int bytes;

    while(1) {

        bytes = server_recv(txmanager.server, &msg, &recv_addr);
        if ( bytes <= 0)
            continue;


        /* Update vector clock */
        vclock_update(port, txmanager.vclock, msg.vclock);
        /* Increment clock */
        vclock_increment(txmanager.port,txmanager.vclock);
        
        /* Handle message */
        switch(msg.type) {
            case BEGINTX:
                printf("Begining a transaction: %d \n",msg.tid);
                if(addTransaction(msg.tid,&recv_addr)){
                    /* log prepared */
                    shitviz_append(txmanager.port, "BEGINTX", txmanager.vclock);

                    txlog_entry_t entry;
                    txentry_init(&entry, LOG_BEGIN, msg.tid, txmanager.vclock);
                    txlog_append(txmanager.txlog, &entry);
                } else {
                    /* log prepared */
                    shitviz_append(txmanager.port, "BEGINTX Failed", txmanager.vclock);
                }
                
                break;
            case COMMIT: {
                printf("Commit tid:%d request\n",msg.tid);
                transaction_t* transaction = findTransaction(msg.tid);
                if (transaction == NULL){
                    printf("Transaction was not found\n");
                } if (transaction->state == PREPARE_STATE){
                    printf("Transaction is already in prepare state\n");
                    continue;
                } else {
                    /* Change state of transaction */
                    transaction->state = PREPARE_STATE;

                    /* Prepare to commit*/
                    message_t msgCommit;
                    message_init(&msgCommit,txmanager.vclock);
                    msgCommit.type = PREPARE_TO_COMMIT;
                    
                    /* Send to workers in transaction */ 
                    sendToAllWorkers(transaction, &msgCommit);
                }
                break;
                }
            case ABORT:{
                printf("Aborting transaction tid:%d",msg.tid);
                
                transaction_t* transaction = findTransaction(msg.tid);
                if (transaction == NULL){
                    printf("Transaction was not found");
                } else {
                    /* Change transaction state */
                    transaction->state = ABORT_STATE;

                    /* Abort transaction*/
                    message_t msg;
                    message_init(&msg,txmanager.vclock);
                    msg.type = ABORT;
                
                    /* Send to workers in transaction */ 
                    sendToAllWorkers(transaction, &msg);

                    /* Log abort */
                    shitviz_append(txmanager.port, "ABORT", txmanager.vclock);

                    txlog_entry_t entry;
                    txentry_init(&entry, LOG_ABORT, msg.tid, txmanager.vclock);
                    txlog_append(txmanager.txlog, &entry);
                }
                break;
                }
            case PREPARED:{
                printf("Node is prepared\n");
                //TODO: Same node 2 prepares D:

                /* Get Transaction */
                transaction_t* transaction = findTransaction(msg.tid);
                /* Check if transaction aborted */
                if(transaction->state == ABORT_STATE){ 
                    message_t msg;
                    message_init(&msg,txmanager.vclock);
                    msg.type = TX_ERROR;
                    msg.value = 1;
                    strcpy(msg.strdata,"Transaction was already aborted\n");

                    /* send error */
                    server_send(txmanager.server,&recv_addr, &msg);
                    continue;
                }
                
                /* Reduce prepare counter */
                transaction->nodeCount--;
                
                printf("%d nodes left to commit tid: %d\n",transaction->nodeCount,transaction->tid);

                /* Check if all workers are prepared */
                if(transaction->nodeCount <=0){
                    printf("Lets commit tid: %d\n",transaction->tid);
                    /* Commit transaction*/
                    transaction->state = COMMIT_STATE;

                    /* Create message */
                    message_t msg;
                    message_init(&msg,txmanager.vclock);
                    msg.type = COMMIT;
                
                    /* Send to workers in transaction */ 
                    sendToAllWorkers(transaction, &msg);

                    /* Log abort */
                    shitviz_append(txmanager.port, "COMMIT", txmanager.vclock);

                    txlog_entry_t entry;
                    txentry_init(&entry, LOG_COMMIT, msg.tid, txmanager.vclock);
                    entry.transaction = transaction->tid;
                    txlog_append(txmanager.txlog, &entry);
                }

                break;
                }
            case JOINTX:{
                printf("Node %d wants to join the party at tid: %d\n",ntohs(recv_addr.sin_port),msg.tid);
                /* Get Transaction */
                transaction_t* transaction = findTransaction(msg.tid);
                if(transaction == NULL){
                    printf("Transaction was not found");
                } else if(transaction->state == PREPARE_STATE || transaction->state == COMMIT_STATE || transaction->state == ABORT_STATE){
                    /*Transaction could not be joined*/
                    message_t msg;
                    message_init(&msg,txmanager.vclock);
                    msg.type = TX_ERROR;
                    msg.value = 1;
                    strcpy(msg.strdata,"Transaction was already completed or is preparing to commit");

                    /* send error */
                    server_send(txmanager.server,&recv_addr, &msg);
                } else if(joinTransaction(transaction,recv_addr)){
                    /* Worker joined */
                    printWorkers(transaction);
                } else {
                    /*Transaction could not be joined*/
                    message_t msg;
                    message_init(&msg,txmanager.vclock);
                    msg.type = TX_ERROR;
                    msg.value = 1;
                    strcpy(msg.strdata,"Transaction could not be joined");

                    /* send error */
                    server_send(txmanager.server,&recv_addr, &msg);
                }
                break;
                }
            default:
                printf("...\n");
                break;
        }
    }
    
    return 0;
}

/* Add transaction to array */
transaction_t* addTransaction(uint32_t tid, struct sockaddr_in* dest_addr){
    int i;
    for (i = 0; i < MAX_TRANSACTIONS; i++){
        transaction_t* transaction = &txmanager.transactions[i];
        /* tid exists */
        if (transaction->tid == tid){
            /* Create error message*/
            message_t msg;
            message_init(&msg,txmanager.vclock);
            msg.type = TX_ERROR;
            msg.value = 1;
            strcpy(msg.strdata,"TID already exists");

            /* send error */
            server_send(txmanager.server,dest_addr, &msg);
            return NULL;
        } else if(transaction->state == COMMIT_STATE || transaction->state == ABORT_STATE || transaction->tid == 0) {
            
            /* Reset Transaction */
            memset(transaction,0,sizeof(transaction_t));

            /* Insert transaction */
            transaction->state = BEGIN_STATE;
            transaction->tid = tid;
            transaction->nodeCount = 1;
            transaction->nodes[0].nid = ntohs(dest_addr->sin_port);
            memcpy(&transaction->nodes[0].address, dest_addr,sizeof (struct sockaddr_in));
            
            printf("Transaction:: tid: %d state: %d\nnode:%d was added successfully\n",transaction->tid, transaction->state,transaction->nodes[0].nid);
            printTransactions();
            return transaction;
        }
    }
    /* Transaction log full */
    printf("No more space in transaction manager\n");
    printTransactions();
    /* Create error message*/
    message_t msg;
    message_init(&msg,txmanager.vclock);
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
            message_t send_msg;
            memcpy(&send_msg,msg,sizeof(message_t));
            printf("Sending %s to %d\n", message_string(msg->type), worker->nid);
            server_send(txmanager.server,&worker->address, &send_msg);
        }
    }
}
/* print transactions */
void printTransactions () {
    int i;
    for(i = 0; i<MAX_TRANSACTIONS; i++){
        transaction_t* transaction = &txmanager.transactions[i];
        if(transaction->tid !=0){
            printf("TID: %d state: %d\n",transaction->tid,transaction->state);
        } else {
            printf("TID: NULL\n");
        }
    }
}
/* Print workers*/
void printWorkers(transaction_t* transaction){
    printf("%d workers at Transaction party: %d\n",transaction->nodeCount,transaction->tid);
    int i;
    for(i = 0; i<MAX_NODES; i++){
        worker_t* worker = &transaction->nodes[i];
        printf("Worker: %d\n",worker->nid);
    }
}


/* Add worker to transaction */
bool joinTransaction(transaction_t* transaction,struct sockaddr_in address){
    uint32_t worker_id = ntohs(address.sin_port);
    int i;
    for(i = 0; i <MAX_NODES; i++){
        worker_t* worker = &transaction->nodes[i];
        /* Add worker */
        if(worker->nid == 0){
            worker->nid = worker_id;
            worker->address = address;
            printf("Welcome to the party %d\n",worker_id);
            transaction->nodeCount++;
            return true;
        }
    }
    printf("Could not join worker %d, party is full\n",worker_id);
    return false;
}
