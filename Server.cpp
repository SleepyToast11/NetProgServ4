//
// Created by jerome on 11/23/23.
//

#include <csignal>
#include <atomic>
#include "Server.h"
#include "tcp-utils.h"

int Server::getSockFd()  {return this->sockfd;}

int Server::startServer(){
    int ret;

    incrementServerNum();
    ret = this->start();
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
