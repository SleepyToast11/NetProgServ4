//
// Created by jerome on 11/23/23.
//

#ifndef NETPROGSERV4_SERVER_H
#define NETPROGSERV4_SERVER_H

class Server {

    public:
        virtual int getPort() = 0;

        int getSockFd();
        static pthread_mutex_t serverNumMutex;
        static pthread_cond_t serverNumCond;

        int startServer();
        int getPassiveSock();
        int acceptSock();

        static int getServerNum();

        ~Server();

    private:

        static void incrementServerNum();
        static void decrementServerNum();

        static std::atomic<int> serverNum;
        int sockfd = -1;
        virtual int start() = 0;
        int backlog = 32;
};

#endif //NETPROGSERV4_SERVER_H
