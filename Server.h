//
// Created by jerome on 11/23/23.
//

#ifndef NETPROGSERV4_SERVER_H
#define NETPROGSERV4_SERVER_H

#include <optional>
#include <map>
#include <memory>


enum Status {
    OK, ERR, FAIL
};

struct messageData {
    Status status;
    int code;
    std::string data;
};

class Message{
public:
    int getMessageSize();

    std::optional<std::unique_ptr<char>> toCString();

    Message();

    Message(enum Status status, int codePara, std::string stringPara);

private:
    std::optional<struct messageData> data;
};

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

        void start();

        ~Server();

    private:

        const size_t maxBufferSizeServer = 1056;
        const int timeoutSeconds = 30;

        static void incrementServerNum();
        static void decrementServerNum();

        int recv_nonblock(char* buf, int *err_ret);
        int non_blocking_send(const char *buffer, const size_t length, int *err_ret) const;

        virtual std::optional<Message> processData(char *buffer, int size);

        static std::atomic<int> serverNum;
        int sockfd = -1;

        int backlog = 32;
};

#endif //NETPROGSERV4_SERVER_H
