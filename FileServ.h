//
// Created by jerome on 11/23/23.
//

#ifndef NETPROGSERV4_FILESERV_H
#define NETPROGSERV4_FILESERV_H

#include <utility>

#include "unordered_map"
#include "Server.h"
#include "SyncServ.h"

class FileServ : public Server{
public:
    int getPort() override;
    static std::unordered_map<int, int> fds;

    explicit FileServ(SyncServ syncServ):
            syncServ(std::move(syncServ)){
    }

private:
    static int port;
    int start() override;
    SyncServ syncServ;
};


#endif //NETPROGSERV4_FILESERV_H
