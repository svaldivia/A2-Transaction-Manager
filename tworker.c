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
        vclock_add(wstate.vclock, wstate.node_id); 
        objstore_sync(wstate.store, wstate.vclock);
    }

    /* dump the object store */
    objstore_dump(wstate.store);

    shitviz_append(port, "Started worker", wstate.vclock);

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

                /* Send begin transaction to the transaction manager */
                server_send_to(wstate.server, wstate.tm_host, wstate.tm_port, &msg);
                
                /* Create log entry */
                txlog_entry_t entry;
                txentry_init(&entry, LOG_BEGIN, wstate.transaction, wstate.vclock);
                txlog_append(wstate.txlog, &entry);
               
                //TODO: If the transaction ID is already used log it to shiviz 
                break;
            }

            case JOINTX: {
                if (wstate.is_active) {
                    printf("ERROR: Transaction already active\n");
                    break;
                }
                tx_manager_spawn(&wstate, (const char*)&msg.strdata, msg.port, msg.tid);
                assert(wstate.server);

                /* Join this worker worker to the given transaction */
                server_send_to(wstate.server, wstate.tm_host, wstate.tm_port, &msg);

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
                printf("A = %d\n", objstore_get_a(wstate.store));
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
                }

                objstore_set_id(wstate.store, msg.strdata);
                objstore_sync(wstate.store, wstate.vclock);
                printf("ID = %s\n", objstore_get_id(wstate.store));
                break;
            }

            case DELAY_RESPONSE:
                /* Create log entry? */
               
                break;

            case CRASH:
                /* Crash worker */
                exit(1); 
                break;

            case COMMIT:
                wstate.do_abort = false;
                wstate.do_commit = true;
                /* ------Phase 1----*/
                /* Send commit request to tmanager */

                /* Wait for prepare to commit message */
                /* Create log entry: Prepared */
               
                /* Send Prepared message to tmanager*/

                /* ------Phase 2----*/
                /* Wait for committed message from tmanager */

                /* When committed received, log to file: committed*/
                break;
            case COMMIT_CRASH:
                /* Create log entry */
                break;

            case ABORT:
                /* Create log entry */
                break;

            case ABORT_CRASH:
                /* Create log entry */
                break;
                 
            case VOTE_ABORT:
                /* Create log entry */
                wstate.do_abort = true;
                wstate.do_commit = false;
               
                break;
        }
    }

    return 0;
}
