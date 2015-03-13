#ifndef COMMON_H
#define COMMON_H

#include <arpa/inet.h>

/* resolve the given hostname and store the address in the node struct */
int resolve_host(char* hostname, struct in_addr* out_addr);

#endif
