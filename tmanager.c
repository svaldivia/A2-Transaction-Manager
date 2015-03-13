#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "msg.h"

void usage(char * cmd) {
    printf("usage: %s  portNum\n", cmd);
}

int main(int argc, char ** argv) 
{
    // This is some sample code feel free to delete it

    unsigned long  port;
    char           logFileName[128];
    int            logfileFD;

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
    snprintf(logFileName, sizeof(logFileName), "TmanagerLog_%d.log", port);

    logfileFD = open(logFileName, O_RDWR | O_CREAT | O_SYNC, S_IRUSR | S_IWUSR );
    if (logfileFD < 0 ) {
      char msg[256];
      snprintf(msg, sizeof(msg), "Opening %s failed", logFileName);
      perror(msg);
      err++;
    }
  }

  if (err) {
    printf("%d transaction manager initialization error%sencountered, program exiting.\n",
	   err, err>1? "s were ": " was ");
    return -1;
  }
  
  
  printf("Starting up Transaction Manager on %d\n", port);
  printf("Port number:              %d\n", port);
  printf("Log file name:            %s\n", logFileName);

  return 0;

}
