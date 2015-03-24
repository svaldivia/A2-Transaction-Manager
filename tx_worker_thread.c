#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "msg.h"
#include "server.h"
#include "tworker.h"

void tx_manager_spawn(worker_state_t* wstate, const char* tm_host, uint32_t tm_port, uint32_t transaction) 
{
    wstate->is_active = true;
    wstate->transaction = transaction;
    wstate->server = NULL;
    wstate->tm_port = tm_port;
    wstate->do_abort = false;
    wstate->do_commit = true;
    strcpy(wstate->tm_host, tm_host);

    /* Set up server :: TMananger*/
    server_alloc(&wstate->server, 0, 10);
    server_listen(wstate->server);

    pthread_t thread;
    pthread_create(&thread, NULL, tx_worker_thread, (void*)wstate); 
}

void* tx_worker_thread(void* params) 
{
    worker_state_t* wstate = (worker_state_t*)params;

    printf("Started TM thread: (%s:%d)\n", wstate->tm_host, wstate->tm_port);

    int bytes = 0;
    int running = true;
    struct sockaddr_in recv_addr;
    message_t msg;

    while(1) 
    {
        bytes = server_recv(wstate->server, &msg, &recv_addr);
        if (bytes <= 0)
            continue;

        /* update vector clock */
        vclock_update(wstate->node_id, wstate->vclock, msg.vclock);
        
        switch(msg.type)
        {
            case PREPARE_TO_COMMIT: {
                message_t vote;
                vclock_increment(wstate->node_id, wstate->vclock);
                message_init(&vote, wstate->vclock);

                /* vote commit? */
                if (wstate->do_commit) {
                    vote.type = PREPARED;
                    shitviz_append(wstate->node_id, "Voting COMMIT", wstate->vclock);
                    server_send_to(wstate->server, wstate->tm_host, wstate->tm_port, &vote);

                    /* log prepared */
                    txlog_entry_t entry;
                    txentry_init(&entry, LOG_PREPARED, wstate->transaction, wstate->vclock);
                    txlog_append(wstate->txlog, &entry);

                    break;
                }

                /* vote abort? */
                if (wstate->do_abort) {
                    vote.type = VOTE_ABORT;
                    shitviz_append(wstate->node_id, "Voting ABORT", wstate->vclock);
                    server_send_to(wstate->server, wstate->tm_host, wstate->tm_port, &vote);

                    /* reset state */
                    wstate->do_abort = false;
                    wstate->do_commit = true;
                    break;
                }

                printf("Dunno what to do for PREPARE TO COMMIT!\n");
                break;
            }

            case ABORT: {
                /* log abort */
                shitviz_append(wstate->node_id, "Abort", wstate->vclock);

                /* increment clock */
                vclock_increment(wstate->node_id, wstate->vclock);

                txlog_entry_t entry;
                txentry_init(&entry, LOG_ABORT, wstate->transaction, wstate->vclock);
                txlog_append(wstate->txlog, &entry);

                /* TODO: fuuck we need to roll back */

                /* exit transaction thread */
                running = false;
                break;
            }

            case COMMIT: {
                /* log commit etc */
                shitviz_append(wstate->node_id, "Commit", wstate->vclock);

                /* increment clock */
                vclock_increment(wstate->node_id, wstate->vclock);
                
                txlog_entry_t entry;
                txentry_init(&entry, LOG_COMMIT, wstate->transaction, wstate->vclock);
                txlog_append(wstate->txlog, &entry);

                /* exit transaction thread */
                running = false;
                break;
            }

            case TX_ERROR: {
                char err_buff[512];
                sprintf(err_buff, "Transaction error: %s", msg.strdata);
                shitviz_append(wstate->node_id, err_buff, wstate->vclock);

                /* if value=1 then exit the transaction thread */
                if (msg.value) 
                    running = false;
                break;
            }
        }
    }
    
    /* clean up */
    printf("Exiting transaction thread\n");
    server_shutdown(&wstate->server);
    wstate->is_active = false;
    return NULL;
}
