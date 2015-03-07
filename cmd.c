
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <errno.h>
#include <strings.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "msg.h"


void usage(char * cmd) {
  printf("usage: %s  portNum groupFileList logFile timeoutValue averageAYATime failureProbability \n",
	 cmd);
}


int main(int argc, char ** argv) {
  
  msgType  msg;
  
  msg.msgID = BEGINTX;
  msg.port = 4456;
  strncpy(msg.strData.hostName, "remote.ugrad.cs.ubc.ca", HOSTLEN);
  
  // The command program
  
  _exit(23);
  
}
