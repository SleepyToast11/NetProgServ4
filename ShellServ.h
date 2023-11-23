//
// Created by jerome on 11/23/23.
//

#ifndef NETPROGSERV4_SHELLSERV_H
#define NETPROGSERV4_SHELLSERV_H

#include "Server.h"

class ShellServ: public Server{

    int getPort() override;


private:
    static int port;
    int start() override;

};


#endif //NETPROGSERV4_SHELLSERV_H
