#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "msg.h"
#include "server.h"
#include "tworker.h"

/* spawns a new transaction thread */
void tx_manager_spawn(worker_state_t* wstate, const char* tm_host, uint32_t tm_port, uint32_t transaction) 
{
    wstate->is_active = true;
    wstate->transaction = transaction;
    wstate->server = NULL;
    wstate->tm_port = tm_port;
    wstate->do_abort = false;
    wstate->do_commit = true;
    strcpy(wstate->tm_host, tm_host);

    /* TODO: restore listening port */
    int listen_port = 0;
    if (wstate->uncertain) 
        listen_port = wstate->txlog->header->tm_listen_port;
    else {
        wstate->txlog->header->tm_port = tm_port;
        strcpy(wstate->txlog->header->tm_host, tm_host);
    }

    /* Set up server :: TMananger*/
    server_alloc(&wstate->server, listen_port, 10);
    server_listen(wstate->server);

    /* store ports & TM address in transaction log */
    wstate->txlog->header->tm_listen_port = wstate->server->port;

    pthread_t thread;
    pthread_create(&thread, NULL, tx_worker_thread, (void*)wstate); 
}

/* the loop that waits for a response while in the uncertain state */
void tx_worker_uncertain(worker_state_t* wstate) 
{
    int bytes = 0;
    int running = true;
    struct sockaddr_in recv_addr;
    message_t msg;

    /* loop and wait for reply */
    while(wstate->uncertain) 
    {
        printf("Waiting for decision from transaction manager...\n");

        /* wait for reply */
        bytes = server_recv(wstate->server, &msg, &recv_addr); 
        if (bytes <= 0) {
            /* ask what the fuck happened again */
            message_t ask_msg;
            message_init(&ask_msg, wstate->vclock);
            ask_msg.type = ASK;
            ask_msg.tid = wstate->transaction;

            server_send_to(wstate->server, wstate->tm_host, wstate->tm_port, &ask_msg);
            continue;
        }

        /* update vector clock */
        vclock_update(wstate->node_id, wstate->vclock, msg.vclock);
        vclock_increment(wstate->node_id, wstate->vclock);
        txlog_write_clock(wstate->txlog, wstate->vclock);

        /* we got a reply! */
        switch(msg.type) 
        {
            case COMMIT: {
                /* log commit etc */
                shitviz_append(wstate->node_id, "Commit", wstate->vclock);

                txlog_entry_t entry;
                txentry_init(&entry, LOG_COMMIT, wstate->transaction, wstate->vclock);
                txlog_append(wstate->txlog, &entry);

                wstate->uncertain = false;
                return;
            }

            case ABORT: {
                /* log abort */
                shitviz_append(wstate->node_id, "Abort", wstate->vclock);

                /* roll back */
                update_rollback(wstate);

                wstate->uncertain = false;
                return;
            }

            case PREPARE_TO_COMMIT: 
                shitviz_append(wstate->node_id, "Unexpected PREPARE in uncertain state", wstate->vclock);
                break;

            default:
                shitviz_append(wstate->node_id, "Unknown command in uncertain state", wstate->vclock);
                break;
        }
    }
}

/* main transaction thread recieve loop */
void* tx_worker_thread(void* params) 
{
    worker_state_t* wstate = (worker_state_t*)params;

    printf("Started TM thread: (%s:%d)\n", wstate->tm_host, wstate->tm_port);

    int bytes = 0;
    int running = true;
    struct sockaddr_in recv_addr;
    message_t msg;

    /* if uncertain, wait */
    if (wstate->uncertain) {
        tx_worker_uncertain(wstate);
        /* we're not actually gonna go into the transaction loop,
         * we just need an update on the current transaction */
        running = false;
    }

    while(running) 
    {
        bytes = server_recv(wstate->server, &msg, &recv_addr); 
        if (bytes <= 0)
            continue;

        /* delay function */
        if (wstate->delay != 0 && wstate->delay != -1000) {
            int sleep_time = abs(wstate->delay);
            sleep(sleep_time);
        }

        /* update vector clock */
        vclock_update(wstate->node_id, wstate->vclock, msg.vclock);
        vclock_increment(wstate->node_id, wstate->vclock);
        txlog_write_clock(wstate->txlog, wstate->vclock);
        
        switch(msg.type)
        {
            case PREPARE_TO_COMMIT: {
                message_t vote;
                message_init(&vote, wstate->vclock);
                vote.tid = wstate->transaction;

                /* vote commit? */
                if (wstate->do_commit) {
                    vote.type = PREPARED;
                    shitviz_append(wstate->node_id, "Voting COMMIT", wstate->vclock);

                    /* log prepared */
                    txlog_entry_t entry;
                    txentry_init(&entry, LOG_PREPARED, wstate->transaction, wstate->vclock);
                    txlog_append(wstate->txlog, &entry);

                    /* negative delay crash */
                    if (wstate->delay < 0) exit(1);

                    server_send_to(wstate->server, wstate->tm_host, wstate->tm_port, &vote);

                    /* we're uncertain! force wait for a reply */
                    wstate->uncertain = true;
                    tx_worker_uncertain(wstate);

                    /* this transaction is finished as soon as we get a reply */
                    running = false;
                    break;
                }

                /* vote abort? */
                if (wstate->do_abort) {
                    vote.type = ABORT;
                    shitviz_append(wstate->node_id, "Voting ABORT", wstate->vclock);

                    /* reset state */
                    wstate->do_abort = false;
                    wstate->do_commit = true;

                    /* negative delay crash */
                    if (wstate->delay < 0) exit(1);

                    server_send_to(wstate->server, wstate->tm_host, wstate->tm_port, &vote);

                    /* we're uncertain! force wait for a reply */
                    wstate->uncertain = true;
                    tx_worker_uncertain(wstate);

                    /* this transaction is finished as soon as we get a reply */
                    running = false;
                    break;
                }

                shitviz_append(wstate->node_id, "Dunno what to do for PREPARE :(", wstate->vclock);
                break;
            }

            case TX_ERROR: {
                char err_buff[512];
                sprintf(err_buff, "Manager Error: %s", msg.strdata);
                shitviz_append(wstate->node_id, err_buff, wstate->vclock);

                /* if value=1 then exit the transaction thread */
                if (msg.value) 
                    running = false;
                break;
            }

            case COMMIT: {
                /* log commit etc */
                shitviz_append(wstate->node_id, "Commit", wstate->vclock);

                txlog_entry_t entry;
                txentry_init(&entry, LOG_COMMIT, wstate->transaction, wstate->vclock);
                txlog_append(wstate->txlog, &entry);

                running = false;
                break;
            }

            case ABORT: {
                /* log abort */
                shitviz_append(wstate->node_id, "Abort", wstate->vclock);

                /* roll back */
                update_rollback(wstate);

                running = false;
                break;
            }

            default:
                shitviz_append(wstate->node_id, "Unknown command", wstate->vclock);
                break;
        }
    }
    
    /* clean up */
    printf("Exiting transaction thread\n");
    server_shutdown(&wstate->server);
    wstate->is_active = false;
    return NULL;
}
