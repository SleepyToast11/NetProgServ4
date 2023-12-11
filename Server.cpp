//
// Created by jerome on 11/23/23.
//

#include <csignal>
#include <atomic>
#include "Server.h"
#include "tcp-utils.h"
#include <boost/assign/list_of.hpp>
#include <boost/unordered_map.hpp>
#include <utility>
#include <sstream>
#include <poll.h>

using boost::assign::map_list_of;

const boost::unordered_map<Status,const char*> statusToString = map_list_of
        (OK, "OK")
        (ERR, "ERR")
        (FAIL, "FAIL");


Message::Message(){
    data = std::nullopt;
}

Message::Message(enum Status status, int codePara, std::string stringPara){
    data = {status, codePara, std::move(stringPara)};
}

//returns size including all spaces and null termination
size_t Message::getMessageSize() const {
    if(!data)
        return 0;

    int returnVal = 0;

    std::string statusString =  statusToString.at(data->status);

    returnVal += statusString.length();
    returnVal += 1; //" " 1 space

    returnVal += std::to_string(data->code).length();
    returnVal += 1; //" " 1 space

    returnVal += data->data.length();
    returnVal += 1; // "\0" 1 null_termination

    return returnVal;
}

int Server::recv_nonblock(char* buf, int *err_ret){
    struct pollfd pollrec;
    pollrec.fd = sockfd;
    pollrec.events = POLLIN;

    *err_ret = 0;

    int polled = poll(&pollrec,1,timeoutSeconds);

    if (polled == 0){
        *err_ret = nodata;
        return -1;
    }

    if (polled == -1){
        *err_ret = errno;
        errno = 0;
        return -1;
    }

    return recv(sockfd ,buf,maxBufferSizeServer,0);
}



int Server::non_blocking_send(const char *buffer, const size_t length, int *err_ret) const{
    struct pollfd pfd;
    int ret;

    pfd.fd = sockfd;
    pfd.events = POLLOUT;

    ret = poll(&pfd, 1, timeoutSeconds);

    if (ret == -1) {
        *err_ret = errno;
        errno = 0;
        return -1;
    }

    else if (ret == 0) {
        *err_ret = nodata;
        return -1;
    }

    if (pfd.revents & POLLOUT) {
        ssize_t bytes_written = write(sockfd, buffer, length);
        if (bytes_written < 0) {
            return -1;
        }
    }
    return 0;
}

std::optional<std::unique_ptr<char>> Message::toCString() const {
    if(!data)
        return std::nullopt;

    std::string returnString = statusToString.at(data->status);
    returnString + " " + std::to_string(data->code);
    returnString + " " + data->data;

    std::unique_ptr<char> returnPtr = std::make_unique<char>(getMessageSize());
    strcpy(returnPtr.get(), returnString.c_str());
    return std::move(returnPtr);
}

int Server::getSockFd()  {return this->sockfd;}

/*When called the server will wait for the client
 * to send something and when that is done, will
 * call the processing, where the command shall be
 * parsed. If a message is to be returned it will be
 * not null. Else, the server will send nothing.
 * All function client-wise are non_blocking.
 * If the client disconnects or times out,
 * the command will return.
 * */

void Server::start() {
    int errRet = 0;
    while(true){
        std::shared_ptr<char> buf = std::make_shared<char>(maxBufferSizeServer);

        int recvSize = recv_nonblock(buf.get(), &errRet);

        if(recvSize <= 0)
            goto exitingServerStart;

        std::optional<Message> message = processData(buf.get(), recvSize);

        if(message){
            int messageSize = message->getMessageSize();
            std::optional<std::shared_ptr<char>> messageBuf = message->toCString();

            if(!messageBuf)
                goto exitingServerStart;

            if(non_blocking_send(messageBuf->get(), messageSize, &errRet) < 0)
                goto exitingServerStart;
        }
    }
    exitingServerStart:
        return;
}

int Server::startServer(){
    int ret;

    incrementServerNum();
    this->start();
    decrementServerNum();

    return ret;
}

int Server::getPassiveSock(){
    if(sockfd < 0){

        int ret = passivesocket(getPort(), backlog);
        sockfd = ret;
        return ret;

    } else
        return sockfd;
}

int Server::acceptSock(){
    if(sockfd < 0)
        return -1;

    struct sockaddr_in client_addr; // the address of the client...
    unsigned int client_addr_len = sizeof(client_addr); // ... and its length

    sockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);

    return sockfd;
};

int Server::getServerNum() { return serverNum; }

Server::~Server() {
    if(sockfd >= 0){
        shutdown(sockfd, SHUT_RDWR);
        close(sockfd);
    }
}

void Server::incrementServerNum(){ ++serverNum; }

void Server::decrementServerNum(){
    --serverNum;
    pthread_cond_broadcast(&serverNumCond);
}

std::atomic<int> Server::serverNum = 0;