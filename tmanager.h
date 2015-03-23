#ifndef TMANAGER_H
#define TMANAGER_H

#include <stdint.h>
#include <stdio.h>

#include "common.h"
#include "server.h"

#define MAX_TRANSACTIONS (4)
enum transaction_state {
    BEGIN_STATE,
    COMMIT_STATE,
    PREPARE_STATE,
    ABORT_STATE
};
typedef enum transaction_state transaction_state;

/* Stores information about an ongoing transaction */
struct transaction_t {
    transaction_state state;
    uint32_t tid;
    uint32_t nodes[MAX_NODES];
    uint32_t nodeCount;
};
typedef struct transaction_t transaction_t;

struct txmanager_t 
{
    server_t*     server;
    FILE*         logfile;
    vclock_t      vclock[MAX_NODES];
    transaction_t transactions[MAX_TRANSACTIONS];
    uint32_t      port;
};

typedef struct txmanager_t txmanager_t;


transaction_t* addTransaction(uint32_t tid,struct sockaddr_in* dest_addr);

#endif
