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

#include "msg.h"
#include "tworker.h"

void usage(char * cmd) {
  printf("usage: %s  portNum\n",
	 cmd);
}

int main(int argc, char ** argv) 
{
// This is some sample code feel free to delete it

unsigned long  port;
char           logFileName[128];
char           dataObjectFileName[128];
int            logfileFD;
int            dataObjectFD;
int            vectorLogFD;
ObjectData     *objData;
int retVal;
struct stat    fstatus;
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
  } else {
    /* got the port number create a logfile name */
    snprintf(logFileName, sizeof(logFileName), "WorkerLog_%d.log", port);

    logfileFD = open(logFileName, O_RDWR | O_CREAT | O_SYNC, S_IRUSR | S_IWUSR );
    if (logfileFD < 0 ) {
      char msg[256];
      snprintf(msg, sizeof(msg), "Opening %s failed", logFileName);
      perror(msg);
      err++;
    }
  }
  
    /* Open/create the data object file */

    snprintf(dataObjectFileName, sizeof(dataObjectFileName), 
	     "WorkerData_%d.data", port);

    dataObjectFD = open(dataObjectFileName, 
		     O_RDWR | O_CREAT | O_SYNC, S_IRUSR | S_IWUSR );
    if (dataObjectFD < 0 ) {
      char msg[256];
      snprintf(msg, sizeof(msg), "Opening %s failed", dataObjectFileName);
      perror(msg);
      err++;
    } else {
      // Open/create the ShiViz log file - Note that we are assuming that if the 
      // log file exists that this is the restart of a worker and hence as part of the 
      // recover process will get its old vector clock value and pick up from there.
      // Consquently we will always be appending to a log file. 
      snprintf(dataObjectFileName, sizeof(dataObjectFileName), 
	       "ShiViz_%d.dat", port);
      
      vectorLogFD = open(dataObjectFileName, 
			 O_WRONLY | O_CREAT | O_APPEND | O_SYNC, S_IRUSR | S_IWUSR );
      if (vectorLogFD < 0 ) {
	char msg[256];
	snprintf(msg, sizeof(msg), "Opening %s failed", dataObjectFileName);
	perror(msg);
	err++;
      }
    }

  if (err) {
    printf("%d transaction manager initialization error%sencountered, program exiting.\n",
	   err, err>1? "s were ": " was ");
    return -1;
  }
  
  
  printf("Starting up transaction worker on %d\n", port);
  printf("Port number:                      %d\n", port);
  printf("Log file name:                    %s\n", logFileName);


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
	   "This is a random ID string %ld");

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
