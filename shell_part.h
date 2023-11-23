#include <string>
#include <stdio.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

#ifndef UNTITLED_SHELL_PART_H
#define UNTITLED_SHELL_PART_H

using namespace std;

struct CommandResult{
    string message;
    int exitstatus;
};


void split(char * command, char * argv[]);
string receive_full_message(int client_socket);
CommandResult exec(char command[]);
void * shellThread(void * arg);
#endif //UNTITLED_SHELL_PART_H

