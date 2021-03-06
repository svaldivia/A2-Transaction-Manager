#ifndef TWORKER_H
#define TWORKER_H 1

#include <sys/time.h>
#include <stdint.h>
#include <stdbool.h>

#include "common.h"
#include "server.h"
#include "txlog.h"
#include "shitviz.h"
#include "objstore.h"

/* represents the state of the local worker.
 * this should definately be thread safe but yeah. */
struct worker_state_t 
{
    int         node_id;
    server_t*   server;
    txlog_t*    txlog;
    vclock_t    vclock[MAX_NODES];
    objstore_t* store;

    uint32_t    tm_port;
    char        tm_host[64];

    /* wat do we need */
    int  transaction;
    int  delay;
    bool is_active;
    bool do_commit;
    bool do_abort;
    bool uncertain;
};

typedef struct worker_state_t worker_state_t;

/** common function for rolling back updates on aborts */
void update_rollback(worker_state_t*);

void tx_manager_spawn(worker_state_t*, const char*, uint32_t, uint32_t tid);
void* tx_worker_thread(void* params);

#endif /* TWORKER_H */
