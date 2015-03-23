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

#include "common.h"
#include "msg.h"
#include "tworker.h"
#include "server.h"


void usage(char * cmd) {
  printf("usage: %s  portNum\n",
	 cmd);
}

int main(int argc, char ** argv) 
{
// This is some sample code feel free to delete it

int            port;
char           logFileName[128];
char           dataObjectFileName[128];
int            logfileFD;
int            dataObjectFD;
int            vectorLogFD;
ObjectData     *objData;
int retVal;
struct stat    fstatus;

/* Check cmd line input*/
if (argc != 2) {
usage(argv[0]);
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
    server_alloc(&server_cmd,port, 10);

    server_listen(server_cmd);
    
    /* Set up server :: TMananger*/
    server_t* server_tman = NULL;
    server_alloc(&server_tman,port, 10);

    server_listen(server_tman);


    uint32_t recv_port;
    message_t msg;

    while(1){
        server_recv(server_cmd,&msg,&recv_port);

        /* Handle Message */
        switch (msg.type){
            case BEGINTX:
                /* Send begin transaction to the transaction manager */
                server_send(server_cmd,msg.strdata,msg.port,&msg);
                
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

if(fstat(dataObjectFD, &fstatus) < 0 ) {
    perror("Filestat failed");
    return -2;
  }

  if (fstatus.st_size < sizeof(ObjectData)) {
      /* File hasn's been mapped in before 
         so we need to make sure there is enough
         space used in the file to hold 
         the data.
      */
      ObjectData  space;
      retVal = write(dataObjectFD, &space, sizeof(ObjectData));
      if (retVal != sizeof(ObjectData)) {
	printf("Some sort of writing error\n");
	return -3;
      }
    }

    objData = mmap(NULL, 512, PROT_WRITE|PROT_READ, MAP_SHARED, dataObjectFD, 0);
  
  if (objData == 0) {
    perror("Object data could not be mapped in");
    return -1;
  }
  


  objData->A = 0x01020304;
  objData->B = 0x98765432;
  gettimeofday(&objData->lastUpdateTime, NULL);
  snprintf(objData->IDstring, sizeof(objData->IDstring), 
	   "This is a random ID string");

  // Fill the vector Clock with  0s and this nodes clock time

  // Increment my clock and log an event saying I started
  objData->vectorClock[0].nodeId = port;
  objData->vectorClock[0].time = 1;

  int i;
  
  for (i = 1; i < MAX_NODES; i++) {
    objData->vectorClock[i].nodeId = 0;
    objData->vectorClock[i].time = 0; 
  }
  
  

  msync(objData, sizeof(ObjectData), MS_SYNC);
  
  retVal = munmap(objData, sizeof(ObjectData));
  if (retVal < 0) {
    perror("Unmap failed");
  }
  return 0;

}
