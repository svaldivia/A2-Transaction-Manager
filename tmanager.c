#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>

#include "common.h"
#include "msg.h"
#include "tmanager.h"
#include "server.h"

int main(int argc, char ** argv) 
{
    char logFileName[128];
    int  logfileFD;
    int  port;

    if (argc != 2) {
        printf("usage: %s portNum\n", argv[0]);
        return -1;
    }

    /* Port & log file name */
    port = atoi(argv[1]);
    snprintf(logFileName, sizeof(logFileName), "TmanagerLog_%d.log", port);

    printf("Port number:              %d\n", port);
    printf("Log file name:            %s\n", logFileName);

    /* Set up server */
    server_t* server = NULL;
    server_alloc(&server, port, 10);
    server_listen(server);

    uint32_t  recv_port;
    message_t msg;
    while(1) {
        server_recv(server, &msg, &recv_port);

        /* Update vector clock */

        /* Handle message */
        switch(msg.type) {
            case BEGINTX:
                break;
        }
    }

    return 0;
}
