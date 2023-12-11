#include "FileSupervisor.h"
#include <mutex>

std::shared_ptr<int> FileSupervisor::readers = std::make_shared<int>(0);
std::shared_ptr<pthread_mutex_t> FileSupervisor::rw_mutex = std::shared_ptr<pthread_mutex_t>
        (new pthread_mutex_t(PTHREAD_MUTEX_INITIALIZER));
std::shared_ptr<pthread_cond_t> FileSupervisor::rw_cond = std::shared_ptr<pthread_cond_t>
        (new pthread_cond_t (PTHREAD_COND_INITIALIZER));
std::unordered_map<std::string, std::shared_ptr<SyncFileAccessor>> FileSupervisor::filesByString =
        std::unordered_map<std::string, std::shared_ptr<SyncFileAccessor>>();
std::unordered_map<int, std::shared_ptr<SyncFileAccessor>> FileSupervisor::filesByInt =
        std::unordered_map<int, std::shared_ptr<SyncFileAccessor>>();


template<typename T>
bool existInMap(const std::unordered_map<T, std::shared_ptr<SyncFileAccessor>>& map, T fd){
    auto element = map.find(fd);
    return element != map.end();
}

std::shared_ptr<SyncFileAccessor> FileSupervisor::getSyncFileAccessorStr(const std::string& name){
    supLock_start_read();
    if(existInMap(filesByString, name)){
        supLock_end_read();
        return filesByString.at(name);
    }
    supLock_end_read();

    addFileAccessor(name);

    return filesByString.at(name);
}

void FileSupervisor::setFileDescriptor(const std::string& name, int fd){
    supLock_start_write();
    FileSupervisor::filesByInt.at(fd) = getSyncFileAccessorStr(name);
    supLock_end_write();
}

std::optional<std::shared_ptr<SyncFileAccessor>> FileSupervisor::getSyncFileAccessorInt(int fd){
    supLock_start_read();
    if(existInMap(filesByInt, fd)) {
        supLock_end_read();
        return filesByInt.at(fd);
    }
    supLock_end_read();
    return std::nullopt;
}

void FileSupervisor::addFileAccessor(const std::string& name) {
    supLock_start_write();
    filesByString.at(name) = std::make_shared<SyncFileAccessor>(name);
    supLock_end_write();
}


void FileSupervisor::supLock_start_read(){

    pthread_mutex_lock(rw_mutex.get());

    *readers += 1;
    pthread_mutex_unlock(rw_mutex.get());
}

void FileSupervisor::supLock_end_read(){
    pthread_mutex_lock(rw_mutex.get());
    *readers -= 1;
    if (*readers == 0)
        pthread_cond_signal(rw_cond.get());

    pthread_mutex_unlock(rw_mutex.get());
}

void FileSupervisor::supLock_start_write(){
    pthread_mutex_lock(rw_mutex.get());
    while (*readers > 0) {
        pthread_cond_wait(rw_cond.get(), rw_mutex.get());
    }
}

void FileSupervisor::supLock_end_write(){
    pthread_cond_signal(rw_cond.get());
    pthread_mutex_unlock(rw_mutex.get());
}