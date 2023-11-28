//
// Created by jerome on 11/23/23.
//

#ifndef NETPROGSERV4_SYNCSERV_H
#define NETPROGSERV4_SYNCSERV_H


#include <utility>
#include <vector>
#include <csignal>
#include <fcntl.h>
#include <iostream>
#include <mutex>
#include <algorithm>

class SyncServ{
    public:
        void checkin(int pipeIn){
            std::lock_guard<std::mutex> lock(peerFdsMutex);
            orderPipe.push_back(pipeIn);

            const char refresher = 0;
            write(mainPipe[1], &refresher, sizeof (char));
        }

        void checkOut(int pipeIn){
            std::lock_guard<std::mutex> lock(peerFdsMutex);
            std::vector<int>::iterator it;
            it = find()
        }

        explicit SyncServ(std::vector<int> socks){
            peersFds = std::move(socks);
            pipe(mainPipe);
        }

    private:
        std::mutex peerFdsMutex;
        std::vector<int> peersFds;
        int mainPipe[2] = {0, 0};
        std::vector<int> orderPipe;
};

#endif //NETPROGSERV4_SYNCSERV_H
