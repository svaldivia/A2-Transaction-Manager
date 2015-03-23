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
#include <sys/mman.h>
#include <stdlib.h>
#include <assert.h>

#include "common.h"
#include "msg.h"
#include "tworker.h"
#include "server.h"


int main(int argc, char ** argv) 
{
    int            port;
    char           logFileName[128];
    int            logfileFD;
    int            vectorLogFD;

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
    printf("Log file name:                    %s\n", logFileName);

    
    /* Check or Open log*/

    /* Get the vector clock + set local clock */

    /* Set up server :: Command*/
    server_t* server_cmd = NULL;
    server_alloc(&server_cmd, port, 10);
    server_listen(server_cmd);

    struct sockaddr_in recv_addr;
    message_t msg;

    while(1){
        server_recv(server_cmd,&msg,&recv_addr);

        /* Handle Message */
        switch (msg.type){
            case BEGINTX:
                /* Send begin transaction to the transaction manager */
                tx_manager_spawn(&wstate, (const char*)&msg.strdata, msg.port);

                assert(wstate.server);
                server_send(wstate.server, wstate.tm_host, wstate.tm_port, &msg);
                
                /* Create log entry */
               
                //TODO: If the transaction ID is already used log it to shiviz 
                break;
            case JOINTX:
                /* Join this worker worker to the given transaction */

                //TODO: Handle success or failure from transaction manager
                break;
            case NEW_A:
                /* Change the value of the A object to a new value*/

                //TODO: Check if transaction is currently happening
                    // If not, change it and save it
               
                break;
            case NEW_B:
                /* Change the value of the B object to a new value*/
               //TODO: Check if transaction is currently happening
                        // If not, change it and save it
                break;
            case NEW_IDSTR:
                /* change the value of the ID string*/

                break;
            case DELAY_RESPONSE:
                /* Create log entry */
               
                break;
            case CRASH:
                /* Crash worker */
                exit(1); 
                break;
            case COMMIT:
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
               
                break;
        }
    }

    return 0;
}
