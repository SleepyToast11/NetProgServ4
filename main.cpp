#include <iostream>
#include <unistd.h>
#include <vector>
#include <cstdlib>
#include <atomic>
#include "tcp-utils.h"
#include "Server.h"

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