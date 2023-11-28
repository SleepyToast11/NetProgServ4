#include "FileServ.h"
#include "Server.h"

int FileServ::getPort(){
    return FileServ::port;
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <fcntl.h>
#include <pthread.h>

#include "fileserv.h"
#include "helper-util.h"
#include "tcp-utils/tcp-utils.h"
#include "main.h"

#define MAX_BUFFER_SIZE 256
#define TIMEOUT 30
#define MAX_FILE_OPEN 128
#define NO_LOCK_LEFT (-5)

#define OK 0
#define ERR 1
#define FAIL 2
#define SETFAIL(status, codeRet) \
    int tempFail = errno;       \
    errno = 0;              \
    *(status) = FAIL;         \
    *(codeRet) = tempFail;

#define SETERR_ENOENT(status, codeRet) \
    *(status) = ERR;         \
    *(codeRet) = ENOENT;

#define SETOK(status, codeRet, code) \
    *(status) = OK;                     \
    *(codeRet) = code;                  \

//macros allow to return inside the function, instead of recursive done() -> worker() bad memory frames usage

#define BEFOREEXIT(t_index) \
    for(int i = 0; i < MAX_FILE_OPEN; ++i){ \
        if(userfd[t_index][i] != 0)         \
            rwlock_close(i, t_index, NULL, NULL);\
    }\
    memset(userfd[t_index], 0, sizeof(int) * MAX_BUFFER_SIZE);

#define GETCOMMAND(fd, buffer, t_index) \
    int err = 0;                                \
    int ret = recv_nonblock(fd, buffer, MAX_BUFFER_SIZE, TIMEOUT * 1000, &err); \
    if(ret == recv_nodata || ret < 1){         \
        BEFOREEXIT(t_index)         \
        shutdown(service->fd, SHUT_RDWR);\
        close(service->fd);              \
        return;                    \
    }

#define SENDRESPONSE(fd, codeRet, status, message, t_index)               \
    char buf[MAX_BUFFER_SIZE];                                                          \
                                                                                        \
    memset(buf, 0, MAX_BUFFER_SIZE);                                                    \
                                                                                        \
    char status_str[10];                                                                \
    int response_size;                                                                  \
    char strcode[256];                                                                                    \
    switch (status) {                                                                  \
        case OK:                                                                            \
            sprintf(status_str, "%s", "OK");                              \
            response_size = sprintf(buf, "%s %d %s\n", status_str, codeRet, message);\
            break;                                                                              \
        case FAIL:                                                                          \
            sprintf(status_str, "%s", "FAIL");                            \
            strncpy(strcode, strerror(codeRet), MAX_BUFFER_SIZE);         \
            response_size = sprintf(buf, "%s %d %s\n", status_str, codeRet, strcode);\
            break;                                                                              \
        case ERR:                                                                           \
            sprintf(status_str, "%s", "ERR");                             \
            if((codeRet) == ENOENT)    {                                    \
                strncpy("File not opened", strerror(codeRet), MAX_BUFFER_SIZE);     \
                response_size = sprintf(buf, "%s %d %s\n", status_str, codeRet, strcode);\
            }                                                             \
            else   {                                                       \
                response_size = sprintf(buf, "%s %d %s\n", status_str, codeRet, message);     \
                }\
            break;                                                        \
            default:                                                      \
            response_size = 0;\
            break;\
    }                                                                                   \
    if (non_blocking_send(fd, buf, response_size, TIMEOUT * 1000) < 0){                 \
         BEFOREEXIT(t_index)                                               \
         return;                                                 \
         }

#define SCANCOMMAND1(fd, buffer, format, command, t_index) \
    char com[MAX_BUFFER_SIZE];                                              \
    if(sscanf(buffer, format, com, command) == EOF){ \
            SENDRESPONSE(fd, EIO, FAIL, "bad command", t_index)        \
            continue;                                         \
            }

#define SCANCOMMAND2(fd, buffer, format, command1, command2, t_index) \
    char com[MAX_BUFFER_SIZE];                                                         \
    if(sscanf(buffer, format, com, command1, command2) == EOF){ \
            SENDRESPONSE(fd, EIO, FAIL, "bad command", t_index)        \
            continue;                                         \
            }

typedef struct {
    int open;
    int readers;
    int writers;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    char *filepath;
} rwlock_t;

std::unordered_map<int, int> FileServ::fds;

pthread_mutex_t file_global_mutex = PTHREAD_MUTEX_INITIALIZER;
int **userfd;
bool DELAY;

void rwlock_open(int indexThread, char *path, int *coderet, int *status){


    pthread_mutex_lock(&file_global_mutex);

    for (int i = 0; i < MAX_FILE_OPEN; ++i) {

        rwlock_t *filelock = (filemutex + i);

        if(!strcmp(path, filelock->filepath)) {

            if(userfd[indexThread][i] == 0)
                filelock->open += 1;

            *coderet = i;
            *status = ERR;

            pthread_mutex_unlock(&file_global_mutex);
            return;
        }
    }

    for (int i = 0; i < MAX_FILE_OPEN; ++i) {

        rwlock_t *filelock = (filemutex + i);

        if(filelock->open < 1) {
            int temp;
            if((temp = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) < 0){
                *coderet = errno;
                errno = 0;
                *status = FAIL;
                pthread_mutex_unlock(&file_global_mutex);
                return;
            }
            filelock->open = 1;
            strcpy(filelock->filepath, path);
            filelock->open = 1;

            int *fd = &userfd[indexThread][i];

            *fd = temp;

            *coderet = i;
            *status = OK;

            pthread_mutex_unlock(&file_global_mutex);
            return;
        }
    }
    *coderet = NO_LOCK_LEFT;
    *status = FAIL;
}

void rwlock_close(int indexlock, int indexthread, int *codeRet, int *status){
    rwlock_t *rw = filemutex + indexlock;
    int *fd = &userfd[indexthread][indexlock];

    pthread_mutex_lock(&file_global_mutex);

    if(*fd == 0){
        pthread_mutex_unlock(&file_global_mutex);
        *codeRet = EBADF;
        *status = FAIL;
        return;
    }
    rw->open--;
    *fd = 0;
    if(rw->open < 1)
        memset(rw->filepath, 0, MAX_BUFFER_SIZE);

    if(close(*fd) < 0) {
        int temp = errno;
        errno = 0;
        *status = FAIL;
        *codeRet = temp;
        pthread_mutex_unlock(&rw->mutex);
        pthread_mutex_unlock(&file_global_mutex);
        *fd = 0;
        return;
    }

    pthread_mutex_unlock(&rw->mutex);
    pthread_mutex_unlock(&file_global_mutex);
    if(codeRet != NULL && status != NULL){
        *codeRet = rw->open;
        *status = OK;
    }
}

void rwlock_start_read(int index){
    rwlock_t *rw = filemutex + index;

    pthread_mutex_lock(&rw->mutex);

    while (rw->writers > 0) {
        pthread_cond_wait(&rw->cond, &rw->mutex);
    }

    rw->readers++;
    pthread_mutex_unlock(&rw->mutex);

}

void rwlock_end_read(int index){
    rwlock_t *rw = filemutex + index;
    pthread_mutex_lock(&rw->mutex);

    rw->readers--;

    if (rw->readers == 0)
        pthread_cond_broadcast(&rw->cond);

    pthread_mutex_unlock(&rw->mutex);
}

void rwlock_start_write(int index){
    rwlock_t *rw = filemutex + index;
    pthread_mutex_lock(&rw->mutex);
    while (rw->readers > 0) {
        pthread_cond_wait(&rw->cond, &rw->mutex);
    }
    rw->writers++;
}

void rwlock_end_write(int index){

    rwlock_t *rw = filemutex + index;

    rw->writers--;
    if(!rw->writers)
        pthread_cond_signal(&rw->cond);
    pthread_mutex_unlock(&rw->mutex);
}

void read_w(int indexthread, char *buf, int indexlock, int size, int *codeRet, int *status){
    rwlock_start_read(indexlock);

    int *fd = &userfd[indexthread][indexlock];

    if(fd == 0) {
        rwlock_end_read(indexlock);
        SETERR_ENOENT(status, codeRet)
        return;
    }

    char printIfD[MAX_BUFFER_SIZE];
    sprintf(printIfD, "Start: Reading index: %d", indexlock);
    printd(printIfD);
    if(DELAY)
        sleep(3);

    int temp;
    if((temp = read(*fd, buf, size)) < 0){
        SETFAIL(status, codeRet)
        rwlock_end_read(indexlock);
        return;
    }
    sprintf(printIfD, "End: Reading index: %d", indexlock);
    printd(printIfD);


    SETOK(status, codeRet, temp)
    rwlock_end_read(indexlock);
}

void seek_w(int indexthread, int offset, int indexlock, int *codeRet, int *status){
    rwlock_start_read(indexlock);

    int *fd = &userfd[indexthread][indexlock];

    if(fd == 0) {
        rwlock_end_read(indexlock);
        SETERR_ENOENT(status, codeRet)
        return;
    }

    if(lseek(*fd, offset, SEEK_CUR) < 0){
        SETFAIL(status, codeRet)
        rwlock_end_read(indexlock);
        return;
    }

    SETOK(status, codeRet, 0);
    rwlock_end_read(indexlock);
}

void write_w(int indexthread, int indexlock, int *codeRet, int *status, char *buf, int size){

    rwlock_start_write(indexlock);
    int *fd = &userfd[indexthread][indexlock];

    if(*fd == 0){
        SETERR_ENOENT(status, codeRet)
        rwlock_end_write(indexlock);
        return;
    }

    char printIfD[MAX_BUFFER_SIZE];
    sprintf(printIfD, "Start: writing index: %d", indexlock);
    printd(printIfD);
    if(DELAY)
        sleep(6);


    if(write(*fd, buf, size) < 0){
        SETFAIL(status, codeRet)
        rwlock_end_write(indexlock);
        return;
    }

    sprintf(printIfD, "End: writing index: %d", indexlock);
    printd(printIfD);

    SETOK(status, codeRet, 0);
    rwlock_end_write(indexlock);
}

int FileServ::start() {

    if (filemutex == 0)
        rwlock_init();

    Service *service = (Service*) serviceVoid;

    int t_index = service->index_t;
    int fd = service->fd;

    char buffer[MAX_BUFFER_SIZE + 2];


    while (1){
        memset(buffer, 0, MAX_BUFFER_SIZE);

        GETCOMMAND(fd, buffer, t_index)

        // Parse the client's request
        char command[MAX_BUFFER_SIZE];
        char filename[MAX_BUFFER_SIZE];
        int identifier, offset, length;
        int codeRet, statusRet;
        char sendbackBuffer[MAX_BUFFER_SIZE];

        memset(sendbackBuffer, 0, MAX_BUFFER_SIZE);
        memset(filename, 0, MAX_BUFFER_SIZE);
        memset(command, 0, MAX_BUFFER_SIZE);

        if (strncmp(buffer, "FOPEN", 5) == 0) {
            SCANCOMMAND1(fd, buffer, "%s %s", filename, t_index)
            int i = 0;
            while(filename[i] != 0){
                if(filename[i] == '/') {
                    SENDRESPONSE(fd, 0, FAIL, "Filepath contains /", t_index)
                    continue;
                }
                i++;
            }
            rwlock_open(t_index, filename, &codeRet, &statusRet);
            SENDRESPONSE(fd, codeRet, statusRet, "FOPEN returned without issue", t_index)
        }

        else if (strncmp(buffer, "FCLOSE", 5) == 0) {
            SCANCOMMAND1(fd, buffer, "%s %d", &identifier, t_index)
            // Close the file and send a response to the client
            rwlock_close(identifier, t_index, &codeRet, &statusRet);
            SENDRESPONSE(fd, codeRet, statusRet, "FCLOSE returned without issue", t_index)
        }

        else if (strncmp(buffer, "FSEEK", 5) == 0) {
            SCANCOMMAND2(fd, buffer, "%s %d %d", &identifier,  &offset, t_index);
            // Seek in the file and send a response to the client
            seek_w(t_index, offset, identifier, &codeRet, &statusRet);
            SENDRESPONSE(fd, codeRet, statusRet, "FSEEK returned without issue", t_index)

        }

        else if (strncmp(buffer, "FREAD", 5) == 0) {
            SCANCOMMAND2(fd, buffer, "%s %d %d", &identifier, &length, t_index);
            read_w(t_index, sendbackBuffer, identifier, length, &codeRet, &statusRet);
            SENDRESPONSE(fd, codeRet, statusRet, sendbackBuffer, t_index)
        }

        else if (strncmp(buffer, "FWRITE", 6) == 0) {
            SCANCOMMAND2(fd, buffer, "%s %d %s", &identifier, command, t_index);
            write_w(t_index, identifier, &codeRet,
                    &statusRet, command, strlen(command));
            SENDRESPONSE(fd, codeRet, statusRet, "FWRITE returned without issue", t_index)
        }

        else {
            SENDRESPONSE(fd, 0, FAIL, "command not recognized", t_index)
        }
    }
}
