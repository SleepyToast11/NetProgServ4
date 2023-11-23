#include <iostream>
#include <unistd.h>
#include <vector>
#include <cstdlib>
#include <atomic>
#include "tcp-utils.h"
#include "Server.h"
#include "vector"

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
    int max_thread = 20;
    int incr_thread = 50;
    bool delay = false;

    while (optind < argc){
           if((opt = getopt(argc, argv, "t:Ds:f:vdT:p:")) != -1) {
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
                default:
                    std::cerr << "Usage: " << argv[0]
                              << " [-t int] [-D] [-s int] [-f int] [-v] [-d] [-T int] [peers...]\n";
                    return EXIT_FAILURE;
            }
        } else {
               peers.push_back(optarg);
               optind ++;
           }
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