#ifndef TMANAGER_H
#define TMANAGER_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "common.h"
#include "server.h"
#include "txlog.h"
#include "shitviz.h"

#define MAX_TRANSACTIONS (4)
enum transaction_state {
    BEGIN_STATE,
    COMMIT_STATE,
    PREPARE_STATE,
    ABORT_STATE
};
typedef enum transaction_state transaction_state;

/* Store worker info */
struct worker_t {
    uint32_t nid;
    struct sockaddr_in address;
};
typedef struct worker_t worker_t;

/* Stores information about an ongoing transaction */
struct transaction_t {
    transaction_state state;
    uint32_t tid;
    worker_t nodes[MAX_NODES];
    uint32_t nodeCount;
};
typedef struct transaction_t transaction_t;

struct txmanager_t 
{
    server_t*     server;
    txlog_t*      txlog;
    vclock_t      vclock[MAX_NODES];
    transaction_t transactions[MAX_TRANSACTIONS];
    uint32_t      port;

    /* test settings */
    bool          commit_crash;
    bool          abort_crash;
};

typedef struct txmanager_t txmanager_t;

/* Add a transaction to queue*/
transaction_t* addTransaction(uint32_t tid,struct sockaddr_in* dest_addr);
/* Find by TID */
transaction_t* findTransaction(uint32_t tid);
/* Send a message to all workers in transaction */
void sendToAllWorkers(transaction_t* transaction, message_t* msg);
/* Print transactions */
void printTransactions ();
/* Add worker to transaction */
bool joinTransaction(transaction_t* transaction, struct sockaddr_in address);
/* Print workers*/
void printWorkers(transaction_t* transaction);
/* Check log for transaction */
void checkTransactionLog(uint32_t tid, struct sockaddr_in* address);
/* Restore transactions from log file */
void restoreTransactions();
#endif
