#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>
#include <netinet/in.h>
 
#include "common.h"
#include "msg.h"

#define CMD_BUFFER_SIZE (512)
#define CMD_SIZE (128)
#define CMD_MAX_ARGS (10)

/* Input buffer */
char cmd_buffer[CMD_BUFFER_SIZE]; 
char last_buffer[CMD_BUFFER_SIZE];

/* Current comand */
char* cmd_args[CMD_SIZE];
int   cmd_arg_count = 0;

int read_command();
void do_begin();
void do_join();
void do_newa();
void do_newb();
void do_newid();
void do_crash();
void do_delay();
void do_commit();
void do_commit_crash();
void do_abort();
void do_abort_crash();
void do_vote_abort();

int main(int argc, char* argv[])
{
    int i = 0;
    for(i = 0; i < CMD_MAX_ARGS; i++) 
        cmd_args[i] = malloc(CMD_SIZE);

    while(1) 
    {
        read_command();
        if (cmd_arg_count == 0)
            continue;

        char* cmd_name = cmd_args[0];

        /* make command name all lowercase */
        int x = 0;
        while(cmd_name[x] != '\0') {
            cmd_name[x] = tolower(cmd_name[x]);
            x++;
        }

        if (strlen(cmd_name) == 0) {
            /* repeat last command */
        }

        /* Handle commands */
        if (strncmp(cmd_name, "begin", 5)        == 0) { do_begin();        continue; }
        if (strncmp(cmd_name, "join", 4)         == 0) { do_join();         continue; }
        if (strncmp(cmd_name, "newa", 4)         == 0) { do_newa();         continue; }
        if (strncmp(cmd_name, "newb", 4)         == 0) { do_newb();         continue; }
        if (strncmp(cmd_name, "newid", 5)        == 0) { do_newid();        continue; }
        if (strncmp(cmd_name, "crash", 5)        == 0) { do_crash();        continue; }
        if (strncmp(cmd_name, "delay", 5)        == 0) { do_delay();        continue; }
        if (strncmp(cmd_name, "commit", 6)       == 0) { do_commit();       continue; }
        if (strncmp(cmd_name, "commitcrash", 11) == 0) { do_commit_crash(); continue; }
        if (strncmp(cmd_name, "abort", 5)        == 0) { do_abort();        continue; }
        if (strncmp(cmd_name, "abortcrash", 10)  == 0) { do_abort_crash();  continue; }
        if (strncmp(cmd_name, "voteabort", 9)    == 0) { do_vote_abort();   continue; }

        if (strncmp(cmd_name, "exit", 4) == 0 || strncmp(cmd_name, "quit", 4) == 0)
            break;

        printf("Unknown command '%s'\n", cmd_name);
    }

    return 0;
}

int read_command() 
{
    printf("> ");

    char* str = fgets(cmd_buffer, CMD_BUFFER_SIZE, stdin);
    if (str == NULL) {
        printf("Nothing read\n");
        exit(-1);
    }

    int i;
    for(i = 0; i < CMD_BUFFER_SIZE; i++) {
        if (str[i] == '\n')
            str[i] = '\0';
        if (str[i] == '\0')
            break;
    }

    /* split args */
    int last = 0;
    int arg = 0;
    int len = 0;

    /* get rid of whitespace */
    i = 0;
    while(isspace(str[i])) {
        i++;
        last++;
    }

    for(; i < CMD_BUFFER_SIZE && str[i] != '\0'; i++) 
    {
        if (arg >= CMD_MAX_ARGS) {
            printf("Maximum arguments reached\n");
            break;
        }

        if (!isspace(str[i]))
            continue;

        len = i - last;
        if (len <= 0)
            continue;

        strncpy(cmd_args[arg], &str[last], len);
        cmd_args[arg][len] = '\0';

        last = i + 1;
        arg++;
    }

    len = i - last;
    if (len > 0) {
        strncpy(cmd_args[arg], &str[last], len);
        cmd_args[arg][len] = '\0';
        arg++;
    }

    cmd_arg_count = arg;

    return 1;
}

/* extend this to a complete message sending function... */
void send_message(char* worker_host, uint32_t worker_port, message_t* msg) 
{
    struct sockaddr_in host;
    if (resolve_host(worker_host, &host.sin_addr) != 0)
        return;

    host.sin_family = AF_INET;
    host.sin_port = htons(worker_port);

    printf("Sending %s to %s:%d\n", message_string(msg), inet_ntoa(host.sin_addr), worker_port);

    /* Convert to network byte order */
    message_to_nbo(msg);

    int sockfd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        printf("Unable to create socket!\n");
        exit(-1);
    }

    size_t sent = sendto(sockfd, (void*)msg, sizeof(message_t), 0, (struct sockaddr*)&host, sizeof(host));
    close(sockfd);
}

void do_begin() 
{
    if (cmd_arg_count != 6) {
        printf("usage: begin [worker_host] [worker_port] [tm_host] [tm_port] [tid]\n");
        return;
    }

    message_t msg;
    message_init(&msg, NULL);
    message_write_begin(&msg, cmd_args[3], atoi(cmd_args[4]), atoi(cmd_args[5]));
    
    send_message(cmd_args[1], atoi(cmd_args[2]), &msg);
}

void do_join() 
{ 
    if (cmd_arg_count != 6) {
        printf("usage: join [worker_host] [worker_port] [tm_host] [tm_port] [tid]\n");
        return;
    }

    message_t msg;
    message_init(&msg, NULL);
    message_write_join(&msg, cmd_args[3], atoi(cmd_args[4]), atoi(cmd_args[5]));
    
    send_message(cmd_args[1], atoi(cmd_args[2]), &msg);
}

void do_newa() 
{ 
    if (cmd_arg_count != 4) {
        printf("usage: newa [worker_host] [worker_port] [new value]\n");
        return;
    }

    message_t msg;
    message_init(&msg, NULL);
    message_write_new_A(&msg, atoi(cmd_args[3]));
    
    send_message(cmd_args[1], atoi(cmd_args[2]), &msg);
}

void do_newb() 
{ 
    if (cmd_arg_count != 4) {
        printf("usage: newb [worker_host] [worker_port] [new value]\n");
        return;
    }

    message_t msg;
    message_init(&msg, NULL);
    message_write_new_B(&msg, atoi(cmd_args[3]));
    
    send_message(cmd_args[1], atoi(cmd_args[2]), &msg);
}

void do_newid() 
{ 
    if (cmd_arg_count != 4) {
        printf("usage: newid [worker_host] [worker_port] [new value]\n");
        return;
    }

    message_t msg;
    message_init(&msg, NULL);
    message_write_new_ID(&msg, cmd_args[3]);
    
    send_message(cmd_args[1], atoi(cmd_args[2]), &msg);
}

void do_crash() 
{ 
    if (cmd_arg_count != 3) {
        printf("usage: crash [worker_host] [worker_port]\n");
        return;
    }

    message_t msg;
    message_init(&msg, NULL);
    message_write_crash(&msg);
    
    send_message(cmd_args[1], atoi(cmd_args[2]), &msg);
}

void do_delay() 
{ 
    if (cmd_arg_count != 4) {
        printf("usage: delay [worker_host] [worker_port] [delay]\n");
        return;
    }

    message_t msg;
    message_init(&msg, NULL);
    message_write_delay(&msg, atoi(cmd_args[3]));
    
    send_message(cmd_args[1], atoi(cmd_args[2]), &msg);
}

void do_commit() 
{ 
    if (cmd_arg_count != 3) {
        printf("usage: crash [worker_host] [worker_port]\n");
        return;
    }

    message_t msg;
    message_init(&msg, NULL);
    message_write_commit(&msg);
    
    send_message(cmd_args[1], atoi(cmd_args[2]), &msg);
}

void do_commit_crash() 
{ 
    if (cmd_arg_count != 3) {
        printf("usage: commitcrash [worker_host] [worker_port]\n");
        return;
    }

    message_t msg;
    message_init(&msg, NULL);
    message_write_commit_crash(&msg);
    
    send_message(cmd_args[1], atoi(cmd_args[2]), &msg);
}

void do_abort() 
{ 
    if (cmd_arg_count != 3) {
        printf("usage: abort [worker_host] [worker_port]\n");
        return;
    }

    message_t msg;
    message_init(&msg, NULL);
    message_write_abort(&msg);
    
    send_message(cmd_args[1], atoi(cmd_args[2]), &msg);
}

void do_abort_crash() 
{ 
    if (cmd_arg_count != 3) {
        printf("usage: abortcrash [worker_host] [worker_port]\n");
        return;
    }

    message_t msg;
    message_init(&msg, NULL);
    message_write_abort_crash(&msg);
    
    send_message(cmd_args[1], atoi(cmd_args[2]), &msg);
}

void do_vote_abort() 
{ 
    if (cmd_arg_count != 3) {
        printf("usage: voteabort [worker_host] [worker_port]\n");
        return;
    }

    message_t msg;
    message_init(&msg, NULL);
    message_write_vote_abort(&msg);
    
    send_message(cmd_args[1], atoi(cmd_args[2]), &msg);
}
