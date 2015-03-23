#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "msg.h"
#include "server.h"
#include "tworker.h"

void tx_manager_spawn(worker_state_t* state, const char* tm_host, uint32_t tm_port) 
{
    state->is_active = true;
    state->server = NULL;
    state->tm_port = tm_port;
    strcpy(state->tm_host, tm_host);

    /* Set up server :: TMananger*/
    server_alloc(&state->server, 0, 10);
    server_listen(state->server);

    pthread_t thread;
    pthread_create(&thread, NULL, tx_worker_thread, (void*)state); 
}

void* tx_worker_thread(void* params) 
{
    worker_state_t* wstate = (worker_state_t*)params;

    printf("Started TM thread: (%s:%d)\n", wstate->tm_host, wstate->tm_port);

    int running = true;
    struct sockaddr_in recv_addr;
    message_t msg;

    while(1) 
    {
        server_recv(wstate->server, &msg, &recv_addr);

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


                    break;
                }

                /* vote abort? */
                if (wstate->do_abort) {
                    vote.type = VOTE_ABORT;
                    shitviz_append(wstate->node_id, "Voting ABORT", wstate->vclock);
                    server_send_to(wstate->server, wstate->tm_host, wstate->tm_port, &vote);
                    break;
                }

                printf("Dunno what to do for PREPARE TO COMMIT!\n");
                break;
            }

            case ABORT: {
                /* log abort */
                shitviz_append(wstate->node_id, "Abort", wstate->vclock);

                /* roll back */

                /* exit transaction thread */
                running = false;
                break;
            }

            case COMMIT: {
                /* log commit etc */
                shitviz_append(wstate->node_id, "Commit", wstate->vclock);

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
