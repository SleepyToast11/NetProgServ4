//
// Created by jerome on 12/9/23.
//

#ifndef NETPROGSERV4_SYNCFILEACCESSOR_H
#define NETPROGSERV4_SYNCFILEACCESSOR_H

#include "FileAccesser.h"
#include <tuple>
#include <memory>

class SyncFileAccessor: public FileAccessor {
public:
    explicit SyncFileAccessor(const FileAccessor &accessor);
    explicit SyncFileAccessor(std::string fileName);
    Message rw_open();
    Message rw_close();
    Message write_w(const char* stuff, size_t stuff_length);
    Message read_w(int bytes);
    Message seek_w(off_t offset);
    static void setPeers(std::vector<std::tuple<std::string, unsigned short>> peer);
private:
    //external
    void seekSyncClient(int peerIndex, off_t offset);
    void writeSyncClient(int peerIndex, const char* stuff, size_t stuff_length);
    void readSyncClient(int peerIndex, Message *messageRet, int bytes);
    void closeSyncClient(int peerIndex);
    void openSyncClient(int peerIndex);
    void e_open();
    void e_close();
    void e_write(const char* stuff, size_t stuff_length); //write should never return a message
    std::vector<Message> e_read(int bytes);
    void e_seek(off_t offset);

    static std::tuple<std::optional<int>, std::optional<std::vector<pid_t>>> peerFork();
    static std::vector<std::tuple<std::string, unsigned short>> peers;
};



#endif //NETPROGSERV4_SYNCFILEACCESSOR_H
