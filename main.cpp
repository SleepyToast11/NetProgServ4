#include <iostream>
#include <unistd.h>
#include <vector>
#include <cstdlib>
#include <atomic>
#include "tcp-utils.h"
#include "Server.h"
#include "vector"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <cstring>

#define IP_ADRESS "127.0.0.1"

void startWorkers(Server server[], int numServer){
    pthread_t workers[MAX_THREADS + 1];
    int args[MAX_THREADS];
    threadService = malloc(sizeof (Service) * MAX_THREADS);
    memset(threadService, 0, sizeof (Service) * MAX_THREADS);

    server_service_t *server = malloc(sizeof (server_service_t));

    server->service = services;
    server->size = numService;

    workers[MAX_THREADS] = pthread_create((workers + MAX_THREADS), NULL, startServer, (void*) server);

    for (int i = 0; i < MAX_THREADS; ++i) {
        args[i] = i;
        pthread_create((workers + i), NULL, worker, (void*) (args + i));
    }
    printf("b");
    for (int i = 0; i < MAX_THREADS + 1; ++i) {
        pthread_join(workers[i], NULL);
    }
    printf("a");
}


int main(int argc, char** argv) {

    int opt;
    std::vector<std::string> peers;

    bool attach = false;
    bool verbose = false;
    int shellPort = 9002;
    int filePort = 9001;
    int peerPort = 9003;
    int max_thread = 20;
    int incr_thread = 50;
    bool delay = false;

    while ((opt = getopt(argc, argv, "t:Ds:f:vdT:p:")) != -1) {
        switch (opt) {
            case 't':
                incr_thread = atoi(optarg);
                break;
            case 'D':
                delay = true;
                break;
            case 's':
                shellPort = atoi(optarg);
                break;
            case 'f':
                filePort = atoi(optarg);
                break;
            case 'v':
                verbose = true;
                break;
            case 'd':
                attach = true;
                break;
            case 'T':
                max_thread = atoi(optarg);
                break;
            case 'p':
                peerPort = atoi(optarg);
            default:
                std::cerr << "Usage: " << argv[0]
                          << " [-t int] [-D] [-s int] [-f int] [-v] [-d] [-T int] [peers...]\n";
                return EXIT_FAILURE;
        }
    }

    for(int index = optind; index < argc; index++){
        peers.push_back(argv[index]);
    }

    std::cout << "Peers for replication are: ";
    for (const auto &peer : peers) {
        std::cout << peer << ' ';
    }
    std::cout << '\n';

    //Shell socket
    int sockFdShell, ret1;
    struct sockaddr_in serverAddrShell;

    //File socket
    int sockFdFile, ret2;
    struct sockaddr_in serverAddrFile;

    //Shell client socket
    int clientSocketShell;
    struct sockaddr_in cliAddrShell;

    //File client socket
    int clientSocketFile;
    struct sockaddr_in cliAddrFile;

    socklen_t addr_size;

    sockFdShell = socket(AF_INET, SOCK_STREAM, 0);
    sockFdFile = socket(AF_INET, SOCK_STREAM, 0);

    if (sockFdShell < 0) {
        printf("Error in connection.\n");
        exit(1);
    }

    if (sockFdFile < 0) {
        printf("Error in connection.\n");
        exit(1);
    }

    printf("Server Shell Socket is created.\n");
    printf("Server File Socket is created.\n");

    std::memset(&serverAddrShell, '\0',sizeof(serverAddrShell));
    std::memset(&serverAddrFile, '\0',sizeof(serverAddrFile));

    opt = 1;

    if (setsockopt(sockFdShell, SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT, &opt,sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(sockFdFile, SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT, &opt,sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    serverAddrShell.sin_family = AF_INET;
    serverAddrShell.sin_port = htons(shellPort);
    serverAddrShell.sin_addr.s_addr = inet_addr(IP_ADRESS);

    serverAddrFile.sin_family = AF_INET;
    serverAddrFile.sin_port = htons(filePort);
    serverAddrFile.sin_addr.s_addr = inet_addr(IP_ADRESS);

    ret1 = bind(sockFdShell,(struct sockaddr*)&serverAddrShell,sizeof(serverAddrShell));
    ret2 = bind(sockFdFile,(struct sockaddr*)&serverAddrFile, sizeof(serverAddrFile));


    if (ret1 < 0) {
        printf("Error in binding.\n");
        exit(1);
    }

    if (ret2 < 0) {
        printf("Error in binding.\n");
        exit(1);
    }


}