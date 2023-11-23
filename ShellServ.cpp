//
// Created by jerome on 11/23/23.
//

#include <sys/socket.h>
#include <cstring>
#include <wait.h>
#include "ShellServ.h"
#include "string"

int ShellServ::getPort(){
    return ShellServ::port;
}

struct CommandResult{
    std::string message;
    int exitstatus;
};

const char* path[] = {"/bin/","/usr/bin/",nullptr};

void split(char * command, char * argv[]){
    int i = 0, j = 1;
    argv[0] = &command[i]; //Pointer on the first word of the command
    while(command[i] != '\0'){
        if(command[i] != ' '){
            i++;

        }
        else{
            command[i] = '\0'; //When we find a space, we replace the space by a "\0" so it knows when the word end and put a pointer on the next character
            argv[j] = &command[++i];
            j++;
        }
    }
    argv[j] = nullptr; //We add a null pointer at the end so execvp can work
}

std::string receive_full_message(int client_socket){
    std::string message_received;
    char buffer;
    bool message_ended = false;

    while(!message_ended){
        ssize_t bytes_received = recv(client_socket, &buffer, 1, 0);
        if(bytes_received > 0){
            if (buffer == '\n' or buffer == '\r'){
                message_ended = true;
            } else {
                message_received += buffer;
            }
        } else if (bytes_received == 0) {
            printf("Connection ended by client\n");
            message_received = "closed";
            return message_received;
        } else {
            printf("recv() failed");
            break;
        }
    }
    return message_received;
}

CommandResult exec(char command[]) {
    std::string final_result = "";
    int exitcode = -1;
    char *argv_command[100];
    int execvperror;
    int pipefd[2];
    int pipefderror[2];

    split(command, argv_command);

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    if (pipe(pipefderror) == -1) {
        perror("pipeerror");
        exit(EXIT_FAILURE);
    }

    pid_t child_pid = fork();
    if (child_pid < 0) {
        printf("Fork error");
    }

    if (child_pid == 0){
        close(pipefd[0]);
        close(pipefderror[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        execvp(argv_command[0], argv_command);
        execvperror = errno;
        write(pipefderror[1], &execvperror, sizeof(execvperror));
        close(pipefderror[1]);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        char result_c;

        close(pipefd[1]);
        close(pipefderror[1]);

        while (read(pipefd[0],&result_c,1) > 0){
            final_result += result_c;
        }

        exitcode = read(pipefderror[0], &execvperror, sizeof(execvperror));
        close(pipefd[0]);
        close(pipefderror[0]);
        printf(final_result.c_str());
        printf("\n");
        bool cmp = true;
        std::string final_result_true = final_result;
        char * token = strtok((char*)final_result.c_str(),":");
        if(strcmp(argv_command[0], token) == 0){
            cmp = false;
        }
        if(!cmp){
            exitcode = -1;
        }
        printf("%d\n", exitcode);
        wait(NULL);
        return CommandResult{final_result_true, exitcode};
    }
}

int ShellServ::start(){
    CommandResult answer;
    int sockFd = this->getSockFd();
    printf("newSocket Shell OK\n");
    std::string message_received;
    bool command_done = false;

    while(1) {

        message_received = receive_full_message(sockFd);

        if (strcmp(message_received.c_str(), "closed") == 0) {
            return -1;
        }

        if (strcmp(message_received.c_str(), "CPRINT") == 0) {

            if (!command_done) {
                send(sockFd, "ERR EIO No command issued\n", 27, 0);
            } else if (answer.exitstatus != 0) {

                std::string final_result;
                final_result = "FAIL " + std::to_string(answer.exitstatus) + " Preceding command failed : " + answer.message;
                const char *final_result_c_array = final_result.c_str();
                send(sockFd, final_result_c_array, strlen(final_result_c_array), 0);

            } else {

                std::string final_result;
                send(sockFd, answer.message.c_str(), strlen(answer.message.c_str()), 0);
                final_result = "OK 0 Command was successful\n";
                const char *final_result_c_array = final_result.c_str();
                send(sockFd, final_result_c_array, strlen(final_result_c_array), 0);

            }
        } else {

            std::string final_result;
            answer = exec(message_received.data());

            if (answer.exitstatus == 0) {
                final_result = "OK " + std::to_string(answer.exitstatus) + " Command was successful\n";
            } else if (answer.exitstatus  == -1) {
                final_result = "ERR : " + answer.message;
            } else {
                final_result = "FAIL " + std::to_string(answer.exitstatus) + " : " + answer.message;            }

            const char *final_result_c_array = final_result.c_str();
            send(sockFd, final_result_c_array, strlen(final_result_c_array), 0);
            command_done = true;
        }
    }
}