#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "msg.h"
#include "tworker.h"
#include "server.h"
#include "txlog.h"
#include "shitviz.h"

int main(int argc, char ** argv) 
{
    int     port;
    char    txlogName[128];
    char    storeName[128];

    worker_state_t wstate;

    /* Check cmd line input*/
    if (argc != 2) {
        printf("usage: %s  portNum\n", argv[0]);
        return -1;
    }

    char * end;
    int err = 0;

    port = strtoul(argv[1], &end, 10);
    if (argv[1] == end) {
        printf("Port conversion error\n");
        err++;
    }  

    printf("Starting up transaction worker on %d\n", port);
    printf("Port number:                      %d\n", port);

    wstate.node_id = port;

    /* Open object store */
    sprintf(storeName, "store_%d.data", wstate.node_id);
    objstore_init(&wstate.store, storeName);
    
    /* Check or Open log*/
    sprintf(txlogName, "txlog_%d.data", wstate.node_id);
    txlog_open(&wstate.txlog, txlogName); 

    /* Get the vector clock + set local clock */
    txlog_read_clock(wstate.txlog, wstate.vclock);
    if (!vclock_has(wstate.vclock, wstate.node_id)) {
        /* brand new object store?? */
        printf("No vector clock stored\n");
        vclock_add(wstate.vclock, wstate.node_id); 
        objstore_sync(wstate.store, wstate.vclock);
    }

    /* dump the object store */
    objstore_dump(wstate.store);

    shitviz_append(port, "Started worker", wstate.vclock);

    /* 
    if we're in PREPARED (uncertain)
    wstate.uncertain = true;
    wstate.is_active = true;
    tx_manager_spawn(&wstate, (const char*)&msg.strdata, msg.port, msg.tid);
    */

    /* Set up server :: Command */
    server_t* server_cmd = NULL;
    server_alloc(&server_cmd, port, 10);
    server_listen(server_cmd);

    int bytes;
    struct sockaddr_in recv_addr;
    message_t msg;

    while(1)
    {
        bytes = server_recv(server_cmd, &msg, &recv_addr);
        if (bytes <= 0)
            continue;

        vclock_increment(wstate.node_id, wstate.vclock);
        txlog_write_clock(wstate.txlog, wstate.vclock);

        /* Handle Message */
        switch (msg.type)
        {
            case BEGINTX: {
                if (wstate.is_active) {
                    printf("ERROR: Transaction already active\n");
                    break;
                }
                tx_manager_spawn(&wstate, (const char*)&msg.strdata, msg.port, msg.tid);
                assert(wstate.server);

                /* create message to send to TM */
                message_t begin;
                message_init(&begin, wstate.vclock);
                begin.type = BEGINTX;
                begin.tid  = msg.tid;

                /* Send begin transaction to the transaction manager */
                server_send_to(wstate.server, wstate.tm_host, wstate.tm_port, &begin);
                
                /* Create log entry */
                txlog_entry_t entry;
                txentry_init(&entry, LOG_BEGIN, wstate.transaction, wstate.vclock);
                txlog_append(wstate.txlog, &entry);
               
                //TODO: If the transaction ID is already used log it to shiviz 
                break;
            }

            case JOINTX: {
                if (wstate.is_active) {
                    printf("ERROR: Worker is already in an active transaction\n");
                    break;
                }
                tx_manager_spawn(&wstate, (const char*)&msg.strdata, msg.port, msg.tid);
                assert(wstate.server);

                message_t join;
                message_init(&join, wstate.vclock);
                join.type = JOINTX;
                join.tid  = msg.tid;

                /* Join this worker worker to the given transaction */
                server_send_to(wstate.server, wstate.tm_host, wstate.tm_port, &join);

                txlog_entry_t entry;
                txentry_init(&entry, LOG_BEGIN, wstate.transaction, wstate.vclock);
                txlog_append(wstate.txlog, &entry);

                break;
            }

            /* Change the value of the A object to a new value */
            case NEW_A: {
                /* if there is an ongoing transaction, do some logging */
                if (wstate.is_active) {
                    txlog_entry_t entry;
                    txentry_init(&entry, LOG_UPDATE, wstate.transaction, wstate.vclock);
                    entry.old_a = objstore_get_a(wstate.store);
                    entry.old_b = objstore_get_b(wstate.store);
                    strcpy(entry.old_id, objstore_get_id(wstate.store));
                    txlog_append(wstate.txlog, &entry);
                }

                objstore_set_a(wstate.store, msg.value);
                objstore_sync(wstate.store, wstate.vclock);

                char strbuff[32];
                snprintf(strbuff, 32, "Set A = %d\n", objstore_get_a(wstate.store));
                shitviz_append(wstate.node_id, strbuff, wstate.vclock);
                break;
            }

            /* Change the value of the B object to a new value*/
            case NEW_B: {
                /* if there is an ongoing transaction, do some logging */
                if (wstate.is_active) {
                    txlog_entry_t entry;
                    txentry_init(&entry, LOG_UPDATE, wstate.transaction, wstate.vclock);
                    entry.old_a = objstore_get_a(wstate.store);
                    entry.old_b = objstore_get_b(wstate.store);
                    strcpy(entry.old_id, objstore_get_id(wstate.store));
                    txlog_append(wstate.txlog, &entry);
                }

                objstore_set_b(wstate.store, msg.value);
                objstore_sync(wstate.store, wstate.vclock);
                printf("B = %d\n", objstore_get_b(wstate.store));

                char strbuff[32];
                snprintf(strbuff, 32, "Set B = %d\n", objstore_get_b(wstate.store));
                shitviz_append(wstate.node_id, strbuff, wstate.vclock);
                break;
            }

            /* change the value of the ID string */
            case NEW_IDSTR: {
                /* if there is an ongoing transaction, do some logging */
                if (wstate.is_active) {
                    txlog_entry_t entry;
                    txentry_init(&entry, LOG_UPDATE, wstate.transaction, wstate.vclock);
                    entry.old_a = objstore_get_a(wstate.store);
                    entry.old_b = objstore_get_b(wstate.store);
                    strcpy(entry.old_id, objstore_get_id(wstate.store));
                    txlog_append(wstate.txlog, &entry);

                    objstore_set_id(wstate.store, msg.strdata);
                }
                else {
                    objstore_set_id(wstate.store, msg.strdata);
                    objstore_sync(wstate.store, wstate.vclock);
                }

                char strbuff[128];
                snprintf(strbuff, 128, "Set ID = %s\n", objstore_get_id(wstate.store));
                shitviz_append(wstate.node_id, strbuff, wstate.vclock);
                break;
            }

            case DELAY_RESPONSE: {
                if (!wstate.is_active) {
                    shitviz_append(wstate.node_id, "Cannot pass delay - no active transaction", wstate.vclock);
                    break;
                }

                message_t delay;
                message_init(&delay, wstate.vclock);
                message_write_delay(&delay, msg.delay);

                shitviz_append(wstate.node_id, "Passing DELAY to manager", wstate.vclock);

                server_send_to(wstate.server, wstate.tm_host, wstate.tm_port, &delay);
                break;
            }

            case CRASH:
                /* Crash worker */
                exit(1); 
                break;

            case COMMIT: {
                if (!wstate.is_active) {
                    shitviz_append(wstate.node_id, "Cannot commit - no active transaction", wstate.vclock);
                    break;
                }

                message_t commit;
                message_init(&commit, wstate.vclock);
                message_write_commit(&commit);
                commit.tid = wstate.transaction;

                shitviz_append(wstate.node_id, "Passing COMMIT to manager", wstate.vclock);
                
                /* Send commit request to tmanager */
                server_send_to(wstate.server, wstate.tm_host, wstate.tm_port, &commit);

                /* Wait for prepare to commit message */
                /* Create log entry: Prepared */
               
                /* Send Prepared message to tmanager*/

                /* ------Phase 2----*/
                /* Wait for committed message from tmanager */

                /* When committed received, log to file: committed*/
                break;
            }

            case COMMIT_CRASH: {
                if (!wstate.is_active) {
                    shitviz_append(wstate.node_id, "Cannot pass commit crash - no active transaction", wstate.vclock);
                    break;
                }

                message_t commit_crash;
                message_init(&commit_crash, wstate.vclock);
                message_write_commit_crash(&commit_crash);
                server_send_to(wstate.server, wstate.tm_host, wstate.tm_port, &commit_crash);
                shitviz_append(wstate.node_id, "Passing COMMIT CRASH to manager", wstate.vclock);
                break;
            }
                               
            /* abort current transaction */
            case ABORT: {
                if (!wstate.is_active) {
                    shitviz_append(wstate.node_id, "Cannot abort - no active transaction", wstate.vclock);
                    break;
                }

                /* log abort */
                /* rollback */

                wstate.do_commit = false;
                wstate.do_abort = true;

                message_t abort;
                message_init(&abort, wstate.vclock);
                message_write_abort(&abort);
                shitviz_append(wstate.node_id, "Aborting transaction", wstate.vclock);
                break;
            }

            case ABORT_CRASH: {
                if (!wstate.is_active) {
                    shitviz_append(wstate.node_id, "Cannot pass abort crash - no active transaction", wstate.vclock);
                    break;
                }

                message_t abort_crash;
                message_init(&abort_crash, wstate.vclock);
                message_write_abort_crash(&abort_crash);
                server_send_to(wstate.server, wstate.tm_host, wstate.tm_port, &abort_crash);
                shitviz_append(wstate.node_id, "Passing ABORT CRASH to manager", wstate.vclock);
                break;
            }
                 
            case VOTE_ABORT: {
                /* Create log entry */
                wstate.do_abort = true;
                wstate.do_commit = false;
                shitviz_append(wstate.node_id, "Will vote ABORT on next PREPARE", wstate.vclock);
                break;
            }

            default:
                printf("unknown command\n"); 
                break;
        }
    }

    return 0;
}
