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
    int  port;

    if (argc != 2) {
        printf("usage: %s portNum\n", argv[0]);
        return -1;
    }

    port = atoi(argv[1]);

    printf("Port number:   %d\n", port);
    printf("Log file name: %s\n", txlogName);

    /* Init vector clock */
    vclock_init(txmanager.vclock);
    
    /* Set up sever */
    txmanager.server = NULL;
    txmanager.port = port;

    /* Check or Open log*/
    sprintf(txlogName, "TmanagerLog_%d.log", txmanager.port);
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
   
    /* Check for rollback */ 
    restoreTransactions();

    struct sockaddr_in recv_addr;
    message_t msg;
    int bytes;

    while(1) 
    {

        bytes = server_recv(txmanager.server, &msg, &recv_addr);
        if (bytes <= 0)
            continue;

        /* Update vector clock */
        vclock_update(port, txmanager.vclock, msg.vclock);
        /* Increment clock */
        vclock_increment(txmanager.port,txmanager.vclock);
        
        /* Handle message */
        switch(msg.type) 
        {
            case ASK: {
                txlog_entry_t entry;
                if (!txlog_last_tx(txmanager.txlog, &entry, msg.tid)) {
                    printf("ASK: Unknown transacion id %d\n", msg.tid);
                    shitviz_append(txmanager.port, "Unknown transaction array", txmanager.vclock);
                    break;
                }

                message_t ask_msg;
                message_init(&ask_msg, txmanager.vclock);
                ask_msg.tid = entry.transaction;
                switch(entry.type) {
                    case LOG_COMMIT:
                        ask_msg.type = COMMIT;
                        server_send(txmanager.server, &recv_addr, &ask_msg);
                        shitviz_append(txmanager.port, "ASK: Commit", txmanager.vclock);
                        break;
                    case LOG_ABORT:
                        ask_msg.type = ABORT;
                        server_send(txmanager.server, &recv_addr, &ask_msg);
                        shitviz_append(txmanager.port, "ASK: Abort", txmanager.vclock);
                        break;
                    default:
                        printf("Transaction %d is not commited/aborted!\n", entry.transaction);
                        break;
                }
                break;
            }

            case BEGINTX: {
                if(txlog_free_id(txmanager.txlog,msg.tid)){
                    if(addTransaction(msg.tid,&recv_addr)){
                        printf("Begining a transaction: %d \n",msg.tid);
                        /* log prepared */
                        shitviz_append(txmanager.port, "BEGINTX", txmanager.vclock);

                        txlog_entry_t entry;
                        txentry_init(&entry, LOG_BEGIN, msg.tid, txmanager.vclock);
                        txlog_append(txmanager.txlog, &entry);
                    } else {
                        printf("Transaction already exits\n"); 
                        /* log prepared */
                        shitviz_append(txmanager.port, "BEGINTX Failed", txmanager.vclock);
                    }
                } else{
                    printf("Transaction already exits\n"); 
                    /* Create error message*/
                    message_t msg;
                    message_init(&msg,txmanager.vclock);
                    msg.type = TX_ERROR;
                    msg.value = 1;
                    strcpy(msg.strdata,"TID already exists");

                    server_send(txmanager.server,&recv_addr, &msg);
                    
                    shitviz_append(txmanager.port, "TID Exists", txmanager.vclock);
                }
                break;
            }

            case COMMIT: {
                printf("Commit tid:%d request\n",msg.tid);
                transaction_t* transaction = findTransaction(msg.tid);
                if (transaction == NULL){
                    /* Check transaction log for status */
                    printf("Transaction was not found\n");
                    checkTransactionLog(msg.tid, &recv_addr);
                } else if (transaction->state == PREPARE_STATE){
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
            case ABORT: {
                printf("Aborting transaction tid:%d",msg.tid);
                
                transaction_t* transaction = findTransaction(msg.tid);
                if (transaction == NULL){
                    /* Check transaction log for status */
                    printf("Transaction was not found");
                    checkTransactionLog(msg.tid,&recv_addr);
                } else {
                    /* Change transaction state */
                    transaction->state = ABORT_STATE;

                    /* Log abort */
                    shitviz_append(txmanager.port, "ABORT", txmanager.vclock);

                    txlog_entry_t entry;
                    txentry_init(&entry, LOG_ABORT, msg.tid, txmanager.vclock);
                    txlog_append(txmanager.txlog, &entry);

                    if (txmanager.abort_crash) {
                        printf("Abort crash\n");
                        exit(1);
                    }

                    /* Abort transaction*/
                    message_t msg;
                    message_init(&msg,txmanager.vclock);
                    msg.type = ABORT;
                
                    /* Send to workers in transaction */ 
                    sendToAllWorkers(transaction, &msg);

                }
                break;
            }
            case PREPARED:{
                printf("Node is prepared\n");
                //TODO: Same node 2 prepares D:

                /* Get Transaction */
                transaction_t* transaction = findTransaction(msg.tid);
                /* Check if transaction exists locally */
                if (transaction == NULL){
                    /* Search transaction result in log */
                    checkTransactionLog(msg.tid,&recv_addr);
                }
                
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


                    /* Log commit */
                    shitviz_append(txmanager.port, "COMMIT", txmanager.vclock);

                    txlog_entry_t entry;
                    txentry_init(&entry, LOG_COMMIT, msg.tid, txmanager.vclock);
                    entry.transaction = transaction->tid;
                    txlog_append(txmanager.txlog, &entry);

                    /* commit crash */
                    if (txmanager.commit_crash) {
                        printf("Commit crash\n");
                        exit(1);
                    }

                    /* Create message */
                    message_t msg;
                    message_init(&msg,txmanager.vclock);
                    msg.type = COMMIT;
                
                    /* Send to workers in transaction */ 
                    sendToAllWorkers(transaction, &msg);
                }

                break;
            }
            case JOINTX: {
                printf("Node %d wants to join the party at tid: %d\n",ntohs(recv_addr.sin_port),msg.tid);
                /* Get Transaction */
                transaction_t* transaction = findTransaction(msg.tid);
                if(transaction == NULL){
                    printf("Transaction was not found\n");
                    
                    /* Check transaction log */
                    checkTransactionLog(msg.tid,&recv_addr);
                } else if(transaction->state == PREPARE_STATE || transaction->state == COMMIT_STATE || transaction->state == ABORT_STATE){
                    /*Transaction could not be joined*/
                    message_t msg;
                    message_init(&msg, txmanager.vclock);
                    msg.type = TX_ERROR;
                    msg.value = 1;
                    strcpy(msg.strdata, "Transaction was already completed or is preparing to commit");

                    /* send error */
                    server_send(txmanager.server,&recv_addr, &msg);
                    
                    shitviz_append(txmanager.port, "JOINTX Failed", txmanager.vclock);
                } else if(joinTransaction(transaction,recv_addr)){
                    /* Worker joined */
                    printWorkers(transaction);

                    shitviz_append(txmanager.port, "Worker joined", txmanager.vclock);
                } else {
                    /*Transaction could not be joined*/
                    message_t msg;
                    message_init(&msg,txmanager.vclock);
                    msg.type = TX_ERROR;
                    msg.value = 1;
                    strcpy(msg.strdata, "Transaction could not be joined");

                    /* send error */
                    server_send(txmanager.server,&recv_addr, &msg);
                    shitviz_append(txmanager.port, "JOINTX failed", txmanager.vclock);
                }
                break;
            }

            case ABORT_CRASH:
                txmanager.abort_crash = true;
                printf("Set abort crash flag\n");
                shitviz_append(txmanager.port, "Set abort crash flag", txmanager.vclock);
                break;

            case COMMIT_CRASH:
                txmanager.commit_crash = true;
                printf("Set commit crash flag\n");
                shitviz_append(txmanager.port, "Commit crash flag", txmanager.vclock);
                break;

            default:
                printf("Unknown command %d\n", msg.type);
                shitviz_append(txmanager.port, "Unknown command", txmanager.vclock);
                break;
        }
    }
    
    return 0;
}

/* Add transaction to array */
transaction_t* addTransaction(uint32_t tid, struct sockaddr_in* dest_addr)
{ 
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
            shitviz_append(txmanager.port, "TID already exists", txmanager.vclock);
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

            shitviz_append(txmanager.port, "Transaction joined", txmanager.vclock);
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
            printf("Sending %s to %d\n", message_string(msg), worker->nid);
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

void restoreTransactions(){
    int i;
    int j;
    int count = txmanager.txlog->header->tx_count;
    txlog_entry_t entry;   
    txlog_entry_t entry_2;
    bool transaction_completed = false;

    printf("Recovering...\n");
    for (i = 0; i < count; i++){
        /* Look for begin entry*/
        printf("%d...",i);
        txlog_read_entry(txmanager.txlog,i,&entry);
        if(entry.type == LOG_BEGIN){
            /* Look for commit or abort */
            transaction_completed = false;
            for(j = i; j < count; j++){ 
                txlog_read_entry(txmanager.txlog,j,&entry_2);
                /*Check tid + if commit or abort*/
                if(entry.transaction == entry_2.transaction && (entry_2.type == LOG_ABORT || entry_2.type == LOG_COMMIT)){
                    transaction_completed = true;
                    break;
                }
            }
            if(!transaction_completed){
                /* Append abort to transaction*/
                txlog_entry_t abort_entry;
                txentry_init(&abort_entry, LOG_ABORT, entry.transaction, txmanager.vclock);
                txlog_append(txmanager.txlog, &abort_entry);

                /* Add to local transactions */

            }
        }
    }
    printf("end of restore\n");
}

/* Check log for transaction */
void checkTransactionLog(uint32_t tid, struct sockaddr_in* recv_addr){
    printf("Checking transaction %d\n",tid);

    txlog_entry_t entry;
    if (!txlog_last_tx(txmanager.txlog, &entry, tid)) {
        printf("Unknown transacion id %d\n", tid);
        return;
    }

    message_t result_msg;
    message_init(&result_msg, txmanager.vclock);
    result_msg.tid = entry.transaction;
    switch(entry.type) {
        case LOG_COMMIT:
            result_msg.type = COMMIT;
            server_send(txmanager.server, recv_addr, &result_msg);
            shitviz_append(txmanager.port, "Commit", txmanager.vclock);
            break;
        case LOG_ABORT:
            result_msg.type = ABORT;
            server_send(txmanager.server, recv_addr, &result_msg);
            shitviz_append(txmanager.port, "Abort", txmanager.vclock);
            break;
        default:
            printf("Transaction %d is not commited/aborted!\n", entry.transaction);
            break;
    }
   printf("Transaction Checked\n"); 
}

