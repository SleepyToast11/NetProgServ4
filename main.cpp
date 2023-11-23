#include <iostream>
#include <unistd.h>
#include <vector>
#include <cstdlib>
#include <atomic>
#include "tcp-utils.h"

class Server{
public:
    virtual int getPort() = 0;

    int getSockFd() const {return this->sockfd;};
    static pthread_mutex_t serverNumMutex;
    static pthread_cond_t serverNumCond;

    int startServer(){
        int ret;

        incrementServerNum();
        ret = this->start();
        decrementServerNum();
        pthread_cond_broadcast(&serverNumCond);

        return ret;
    };

    int getPassiveSock(){
        if(sockfd < 0){

            int ret = passivesocket(getPort(), backlog);
            sockfd = ret;
            return ret;

        } else
            return sockfd;
    }

    int acceptSock(){
        if(sockfd < 0)
            return -1;

        struct sockaddr_in client_addr; // the address of the client...
        unsigned int client_addr_len = sizeof(client_addr); // ... and its length

        sockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);

        return sockfd;
    };

    static int getServerNum() { return serverNum; };

    ~Server() {
        if(sockfd >= 0){
            shutdown(sockfd, SHUT_RDWR);
            close(sockfd);
        }
    }

private:

    static void incrementServerNum(){ ++serverNum; }
    static void decrementServerNum(){ --serverNum; }
    static std::atomic<int> serverNum;
    int sockfd = -1;
    virtual int start() = 0;
    int backlog = 32;
    int startPassiveSock(){
        int port = this->getPort();
        passivesocket(port, backlog);
    }
};

std::atomic<int> Server::serverNum = 0;

class fileServ: public Server{
public:

private:
    static int port;
    static int fd;
    void setSockFd(int sock) override{
        fileServ::fd = sock;
    }
};



int main(int argc, char** argv) {

    int opt;
    std::vector<std::string> peers;

    {
        bool attach = false;
        bool verbose = false;
        int shellPort = 9002;
        int filePort = 9001;
        int max_tread = 5;
        bool delay = false;

    }

    while ((opt = getopt(argc, argv, "t:Ds:f:vdT:p:")) != -1) {
        switch (opt) {
            case 't':
                std::cout << "Option -t with value " << atoi(optarg) << '\n';
                break;
            case 'D':
                std::cout << "Option -D\n";
                break;
            case 's':
                std::cout << "Option -s with value " << atoi(optarg) << '\n';
                break;
            case 'f':
                std::cout << "Option -f with value " << atoi(optarg) << '\n';
                break;
            case 'v':
                std::cout << "Option -v\n";
                break;
            case 'd':
                std::cout << "Option -d\n";
                break;
            case 'T':
                std::cout << "Option -T with value " << atoi(optarg) << '\n';
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " [-t int] [-D] [-s int] [-f int] [-v] [-d] [-T int] [peers...]\n";
                return EXIT_FAILURE;
        }
    }

    for (int index = optind; index < argc; index++) {
        peers.push_back(argv[index]);
    }

    std::cout << "Peers for replication are: ";
    for (const auto &peer : peers) {
        std::cout << peer << ' ';
    }
    std::cout << '\n';

    return EXIT_SUCCESS;

    int bgpid = fork();
    if (bgpid < 0) {
        perror("startup fork");
        return 1; }
    if (bgpid) // parent dies
        return 0;
}