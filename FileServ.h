//
// Created by jerome on 11/23/23.
//

#ifndef NETPROGSERV4_FILESERV_H
#define NETPROGSERV4_FILESERV_H

#include <utility>
#include "FileSupervisor.h"
#include "Server.h"
#include <unordered_map>


class FileServ : public Server{
public:
    static void setFileSupervisor(std::shared_ptr<FileSupervisor> fileSupervisor);
    FileServ(FileServ &t);
    void start() override;
    static void setPort(int port);
private:
    int getPort() override;
    static int port;
    static std::shared_ptr<FileSupervisor> files;
};


#endif //NETPROGSERV4_FILESERV_H
