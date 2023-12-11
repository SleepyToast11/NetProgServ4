#include "SyncFileAccessor.h"
#include <fcntl.h>
#include "tcp-utils.h"
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include "Server.h"
#include <stdio.h>
#include <string.h>

#define CHECKOPEN \
if(!open)      \
return Message(ERR, ENOENT, "File not opened");

void SyncFileAccessor::setPeers(std::vector<std::tuple<std::string, unsigned short>> peer){
    peers = std::move(peer);
}

SyncFileAccessor::SyncFileAccessor(std::string fileName) : FileAccessor(std::move(fileName)) {}


//forks a process for every peer
std::tuple<std::optional<int>, std::optional<std::vector<pid_t>>> SyncFileAccessor::peerFork(){
    size_t peerNum = peers.size();
    std::optional<std::vector<pid_t>> clientPids;

    for (int i = 1; i < peerNum; ++i) {
        clientPids->push_back(fork());

        if(clientPids->back() < 0)
            exit(-1); //if in infinite loop, better shut down the server to avoid crashing all other programs

        if(clientPids->back() == 0)
            return std::tuple<std::optional<int>, std::optional<std::vector<pid_t>>>{i, std::nullopt};  //Parent continue to create childs
    }
    return std::tuple<std::optional<int>, std::optional<std::vector<pid_t>>>{std::nullopt, clientPids};
}

//careful, blocking call. makes sure all calls have timeout or will stay blocked
void waitPeers(const std::vector<pid_t>& clientPid){
    for (pid_t pid: clientPid) {
        int status;
        waitpid(pid, &status, 0);
    }
}

std::vector<std::shared_ptr<Message>> SyncFileAccessor::e_read(int bytes){
    std::vector<std::shared_ptr<Message>> retMessage = std::vector<std::shared_ptr<Message>>();
    std::tuple<std::optional<int>, std::optional<std::vector<pid_t>>> pidRet = peerFork();
    if(get<0>(pidRet).has_value()) {
        int peerIndex = get<0>(pidRet).value();
        readSyncClient(peerIndex, retMessage[peerIndex], bytes);
        exit(1); //This should never be reached
    }
    waitPeers(get<1>(pidRet).value());
    return retMessage;
}

void SyncFileAccessor::e_seek(off_t offset){
    std::tuple<std::optional<int>, std::optional<std::vector<pid_t>>> pidRet = peerFork();
    if(get<0>(pidRet).has_value()) {
        int peerIndex = get<0>(pidRet).value();
        seekSyncClient(peerIndex, offset);
        exit(1); //This should never be reached
    }
}

void SyncFileAccessor::e_open(){
    std::tuple<std::optional<int>, std::optional<std::vector<pid_t>>> pidRet = peerFork();
    if(get<0>(pidRet).has_value()) {
        int peerIndex = get<0>(pidRet).value();
        openSyncClient(peerIndex);
        exit(1); //This should never be reached
    }
}

void SyncFileAccessor::e_close(){
    std::tuple<std::optional<int>, std::optional<std::vector<pid_t>>> pidRet = peerFork();
    if(get<0>(pidRet).has_value()) {
        int peerIndex = get<0>(pidRet).value();
        closeSyncClient(peerIndex);
        exit(1); //This should never be reached
    }
}

void SyncFileAccessor::e_write(const char *stuff, size_t stuff_length) {
    std::tuple<std::optional<int>, std::optional<std::vector<pid_t>>> pidRet = peerFork();
    if(get<0>(pidRet).has_value()) {
        int peerIndex = get<0>(pidRet).value();
        writeSyncClient(peerIndex, stuff, stuff_length);
        exit(1); //This should never be reached
    }
}

Message SyncFileAccessor::read_w(int bytes){
    CHECKOPEN
    rwlock_start_read();
    std::vector<std::shared_ptr<Message>> allBytes = e_read(bytes);

    allBytes.push_back(std::make_shared<Message>(FileAccessor::read_w(bytes)));
    size_t max_length = 0;

    int failCounter = 0;
    int okCounter = 0;
    // Find the maximum length of the strings in the vector
    for (const std::shared_ptr<Message>& message: allBytes) {
        if(!message->data.has_value())
            continue;
        (message->data->status == OK) ? okCounter++ : failCounter++;
        max_length = std::max(max_length, message->data->data.size());
    }

    if(failCounter > okCounter) {
        e_seek(-bytes);
        e_write(allBytes.back()->data->data.c_str(), allBytes.back()->data->data.size());
        return {FAIL, bytes, "Most peers failed to sync. Trying to restore peers using local data"};
    }
    std::string result;
    // Iterate over each index
    for (size_t i = 0; i < max_length; ++i) {
        std::vector<int> counts(256, 0);

        // Count the characters at the current index
        for (const std::shared_ptr<Message>& message: allBytes) {
            std::string str = message->data->data;
            if (i < str.size()) {
                ++counts[static_cast<unsigned char>(str[i])];
            }
        }

        // Find the character with the maximum count
        char most_common = '\0';
        int max_count = 0;
        for (size_t j = 0; j < counts.size(); ++j) {
            if (counts[j] > max_count) {
                max_count = counts[j];
                most_common = static_cast<char>(j);
            }
        }

        if(most_common == '\0'){
            result += most_common;
            break;
        }
        // Add the most common character to the result string
        result += most_common;
    }
    rwlock_end_read();
    return {OK, static_cast<int>(result.size()), result};
}

Message SyncFileAccessor::rw_open(){
    e_open();
    return FileAccessor::rw_open();
}

Message SyncFileAccessor::rw_close(){
    CHECKOPEN
    e_close();
    return FileAccessor::rw_close();
}

Message SyncFileAccessor::write_w(const char* stuff, size_t stuff_length){
    CHECKOPEN
    e_write(stuff, stuff_length);
    return FileAccessor::write_w(stuff, stuff_length);
}

Message SyncFileAccessor::seek_w(off_t offset){
    CHECKOPEN
    e_seek(offset);
    return FileAccessor::seek_w(offset);
}

SyncFileAccessor::SyncFileAccessor(const FileAccessor &accessor) : FileAccessor(accessor) {}

void SyncFileAccessor::readSyncClient(int peerIndex, std::shared_ptr<Message> messageRet, int bytes){
    std::tuple<std::string, unsigned short> peer = peers[peerIndex];
    int sd = connectbyportint((get<0>(peer)).c_str(), get<1>(peer));
    if(sd < 0) {
        close(sd);
        *messageRet = Message(SYNC_FAIL, 0, "");
        return;
    }

    Message sendMessage = Message(SYNC_READ, bytes, fileName);

    if(non_blocking_send(sd, sendMessage.toCString().value().get(), (size_t) sendMessage.getMessageSize()) < 0){
        close(sd);
        *messageRet = Message(SYNC_FAIL, 0, "");
        return;
    }

    size_t bufSize = 1024;
    char retBuf[bufSize + 1];
    memset(retBuf, 0, bufSize+1);
    int recvRet;

    int recvSize = recv_nonblock(sd, retBuf, bufSize, &recvRet);

    if(recvSize < 0){
        close(sd);
        *messageRet = Message(SYNC_FAIL, 0, "");
        return;
    }
    close(sd);
    *messageRet = Message(OK, recvSize, std::string (retBuf));
    close(sd);
    *messageRet = Message(SYNC_FAIL, 0, "");
}

void SyncFileAccessor::seekSyncClient(int peerIndex, off_t offset){
    std::tuple<std::string, unsigned short> peer = peers[peerIndex];
    int sd = connectbyportint((get<0>(peer)).c_str(), get<1>(peer));
    if(sd < 0) {
        close(sd);
        return;
    }

    Message sendMessage = Message(SYNC_SEEK, 0, fileName);

    if(non_blocking_send(sd, sendMessage.toCString().value().get(), (size_t) sendMessage.getMessageSize()) < 0){
        close(sd);
        return;
    }
    close(sd);
}

void SyncFileAccessor::writeSyncClient(int peerIndex, const char* stuff, size_t stuff_length){
    std::tuple<std::string, unsigned short> peer = peers[peerIndex];
    int sd = connectbyportint((get<0>(peer)).c_str(), get<1>(peer));
    if(sd < 0) {
        close(sd);
        return;
    }
    std::string sendMessageData = fileName;
    sendMessageData += std::string(stuff);

    Message sendMessage = Message(SYNC_WRITE, fileName.size(), sendMessageData);

    if(non_blocking_send(sd, sendMessage.toCString().value().get(), (size_t) sendMessage.getMessageSize()) < 0){
        close(sd);
        return;
    }
    close(sd);
}

void SyncFileAccessor::closeSyncClient(int peerIndex){
    std::tuple<std::string, unsigned short> peer = peers[peerIndex];
    int sd = connectbyportint((get<0>(peer)).c_str(), get<1>(peer));
    if(sd < 0) {
        close(sd);
        return;
    }

    Message sendMessage = Message(SYNC_CLOSE, fileName.size(), fileName);

    if(non_blocking_send(sd, sendMessage.toCString().value().get(), (size_t) sendMessage.getMessageSize()) < 0){
        close(sd);
        return;
    }
    close(sd);
}

void SyncFileAccessor::openSyncClient(int peerIndex){
    std::tuple<std::string, unsigned short> peer = peers[peerIndex];
    int sd = connectbyportint((get<0>(peer)).c_str(), get<1>(peer));
    if(sd < 0) {
        close(sd);
        return;
    }

    Message sendMessage = Message(SYNC_OPEN, fileName.size(), fileName);

    if(non_blocking_send(sd, sendMessage.toCString().value().get(), (size_t) sendMessage.getMessageSize()) < 0){
        close(sd);
        return;
    }
    close(sd);
}