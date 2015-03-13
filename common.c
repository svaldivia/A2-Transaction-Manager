#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

int resolve_host(char* host, struct in_addr* out_addr)
{
    int error;
    struct addrinfo* result;
    struct sockaddr_in* addr;
    struct addrinfo hints;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    
    error = getaddrinfo(host, NULL, &hints, &result);
    if (error != 0) {
        /* blow up */
        fprintf(stderr, "Failed resolving hostname %s\n", host);
        exit(-1);
    }

    /* copy address to result */
    addr = (struct sockaddr_in*)result->ai_addr;
    memcpy(out_addr, &addr->sin_addr, sizeof(struct in_addr));

    freeaddrinfo(result);
    return 0;
}

void vclock_update(vclock_t* mine, vclock_t* other)
{
} 
