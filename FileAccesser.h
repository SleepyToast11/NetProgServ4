//
// Created by jerome on 12/5/23.
//

#ifndef NETPROGSERV4_FILEACCESSER_H
#define NETPROGSERV4_FILEACCESSER_H
#include <atomic>
#include <vector>
#include "Server.h"
#include <unordered_map>
#include "SyncServ.h"
#include "readerWriter.h"
#include "SyncFileAccessor.h"


class FileAccessor: public readerWriter{
public:
    Message rw_open();
    Message rw_close();
    Message write_w(const char* stuff, size_t stuff_length);
    Message read_w(int bytes);
    Message seek_w(off_t offset);
    explicit FileAccessor(std::string fileName);
    FileAccessor(FileAccessor const &accessor);
private:

    int fd = -1;
    Message i_open();
    Message i_close();
    Message i_write(const char* stuff, size_t stuff_length) const; //write should never return a message
    Message i_read(int bytes) const;
    Message i_seek(off_t offset) const;
    int owners = 0;
    std::string fileName;
    bool opened = false;
    int MAX_LEN = 1024;
};


#endif //NETPROGSERV4_FILEACCESSER_H