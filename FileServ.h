//
// Created by jerome on 11/23/23.
//

#ifndef NETPROGSERV4_FILESERV_H
#define NETPROGSERV4_FILESERV_H

#include "Server.h"

class FileServ : public Server{

    int getPort() override;


private:
    static int port;
    int start() override;
};


#endif //NETPROGSERV4_FILESERV_H
