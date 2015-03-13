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

#include "common.h"
#include "msg.h"
#include "tmanager.h"

int main(int argc, char ** argv) 
{
    char logFileName[128];
    int  logfileFD;
    int  port;

    if (argc != 2) {
        printf("usage: %s  portNum\n", argv[0]);
        return -1;
    }

    port = atoi(argv[1]);

    snprintf(logFileName, sizeof(logFileName), "TmanagerLog_%d.log", port);

    logfileFD = open(logFileName, O_RDWR | O_CREAT | O_SYNC, S_IRUSR | S_IWUSR );
    if (logfileFD < 0 ) {
        printf("Could not open log file %s\n", logFileName);
        return -1;
    }

    printf("Starting up Transaction Manager\n"); 
    printf("Port number:              %d\n", port);
    printf("Log file name:            %s\n", logFileName);

    /* Set up server */

    return 0;
}
