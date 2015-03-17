#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>


/* create server instance */
int server_alloc(server_t** server_ptr, int port, int timeout){
    server_t* server = malloc(sizeof(server_t));
    if (server == NULL) {
        printf("Failed to allocate server struct\n");
        exit(-1);
    }

    /* Set server info */
    server->port = port;
    server->timeout = timeout;
    
    /* Setup socket address */
    memset((void*)&server->address, 0, sizeof(struct sockaddr_in));
    server->address.sin_family = AF_INET;
    server->address.sin_port = htons(server->port);
    
    /* Resolve Host */
    resolve_host("localhost",&server->address.sin_addr);

    *server_ptr = server;
    return 0;
}

/* Bind the socket to the server */
int server_listen(server_t* server){
   struct sockaddr_in serverAddr;
    int sockfd;

    /* Create UDP socket */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket\n");
        return 0;
    }
    
    /* bind socket to a any valid ip */

    memset((char *)&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;   
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(server->port);

    /* Bind server address to socket */
    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind failed");
        return sockfd;
    }
    
    /* Add socket file descriptor to server struct */
    server->socket_id = sockfd;
    return sockfd;
}

/* shuts down the server and frees its memory */
void server_shutdown(server_t** server_ptr){
    if (server_ptr == NULL)
        return;
    server_t* server = *server_ptr;

    /* close socket & log file */
    close(server->socket_id);

    /* free memory */
    free(server);
    *server_ptr = NULL;
}

/* send to address */
int server_send(server_t* server, char* dest_host, uint32_t dest_port, message_t* msg){

    /* */
    struct in_addr dest_address;
    resolve_host(dest_host, &dest_address);

    /* set message information */
    /* TODO */

    /* sendto */ 
    int numbytes;
    if ((numbytes = sendto(server->socket_id,(void*)msg, sizeof(message_t), 0,(struct sockaddr *) &dest_address, sizeof(dest_address)) == -1)) {
        perror("server: send");         
        exit(1);                  
    }
	return 0;
}

/* recieve from node */
int  server_recv(server_t* server, message_t* message, uint32_t* recv_port){
    return server_recv_timeout(server,message, server->timeout);
}
int  server_recv_timeout(server_t* server, message_t* message, uint32_t* recv_port, int timeout)
{
    assert(server != NULL);
    assert(message != NULL);
    assert(recv_port != NULL);
    *recv_port = 0;
    
    ssize_t len;
    struct sockaddr_in sender_addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int flags = 0;

    /* set up file descriptor set */
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(server->socket_id, &fds);

    /* setup timeout struct */
    struct timeval timeout_val;
    timeout_val.tv_sec = timeout;
    timeout_val.tv_usec = 0;

    int n = select(server->socket_id + 1, &fds, NULL, NULL, &timeout_val);
    if (n == 0) {
        /* timeout */
        return -1;
    }
    if (n == -1) {
        fprintf(stderr, "Select error, wtf!\n");
        exit(-1);
    }

    len = recvfrom(server->socket_id, (void*)message, sizeof(message_t), flags, (struct sockaddr*)&sender_addr, &addr_size);
    
    /* Pass sender port */
    *recv_port = ntohs(sender_addr.sin_port);

    /* Convert from network byte order */
    message_from_nbo(message);

    return (int)len;
}
