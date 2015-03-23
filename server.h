#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <sys/time.h>
#include "msg.h"
#include "common.h"

struct server_t 
{
    int port;
    int socket_id;
    int timeout;
    
    /* Connection info */
    struct sockaddr_in address;
    //unsigned short port;
};

typedef struct server_t server_t;

/* create server instance */
int server_alloc(server_t** server_ptr, int port, int timeout);

/* Bind the socket to the server */
int server_listen(server_t* server);

/* shuts down the server and frees its memory */
void server_shutdown(server_t**);

/* send to node */
int server_send(server_t* server, char* dest_host, uint32_t dest_port, message_t* msg);

/* recieve from node */
int  server_recv(server_t* server, message_t* message, struct sockaddr_in* recv_addr);
int  server_recv_timeout(server_t* server, message_t* message, struct sockaddr_in* recv_addr, int timeout);

#endif
