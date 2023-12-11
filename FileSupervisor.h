//
// Created by jerome on 12/9/23.
//

#ifndef NETPROGSERV4_FILESUPERVISOR_H
#define NETPROGSERV4_FILESUPERVISOR_H

#include <string>
#include "SyncFileAccessor.h"
#include <memory>




class FileSupervisor: public readerWriter{
public:

    static std::shared_ptr<SyncFileAccessor> getSyncFileAccessorStr(const std::string& name);
    static std::optional<std::shared_ptr<SyncFileAccessor>> getSyncFileAccessorInt(int fd);
    static void setFileDescriptor(const std::string& name, int fd);
private:
    static void supLock_start_read();
    static void supLock_end_read();
    static void supLock_start_write();
    static void supLock_end_write();
    static std::shared_ptr<int> readers;
    static std::shared_ptr<pthread_mutex_t> rw_mutex;
    static std::shared_ptr<pthread_cond_t> rw_cond;
    static void addFileAccessor(const std::string& name);
    static std::unordered_map<std::string, std::shared_ptr<SyncFileAccessor>> filesByString;
    static std::unordered_map<int, std::shared_ptr<SyncFileAccessor>> filesByInt;
};



#endif //NETPROGSERV4_FILESUPERVISOR_H
