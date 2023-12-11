//
// Created by jerome on 12/9/23.
//

#include "readerWriter.h"

readerWriter::readerWriter() {
    rw_mutex = std::make_shared<pthread_mutex_t>() ;
    rw_cond = std::make_shared<pthread_cond_t>();
    *rw_mutex = PTHREAD_MUTEX_INITIALIZER;
    *rw_cond = PTHREAD_COND_INITIALIZER;
    readers = std::make_shared<int>();
    *readers = 0;
}

readerWriter::readerWriter(const readerWriter& other) {
    this->rw_mutex = other.rw_mutex;
    this->rw_cond = other.rw_cond;
    this->readers = other.readers;
}

void readerWriter::rwlock_start_read(){

    pthread_mutex_lock(rw_mutex.get());

    *readers += 1;
    pthread_mutex_unlock(rw_mutex.get());
}

void readerWriter::rwlock_end_read(){
    pthread_mutex_lock(rw_mutex.get());
    *readers -= 1;
    if (*readers == 0)
        pthread_cond_signal(rw_cond.get());

    pthread_mutex_unlock(rw_mutex.get());
}

void readerWriter::rwlock_start_write(){
    pthread_mutex_lock(rw_mutex.get());
    while (*readers > 0) {
        pthread_cond_wait(rw_cond.get(), rw_mutex.get());
    }
}

void readerWriter::rwlock_end_write(){
    pthread_cond_signal(rw_cond.get());
    pthread_mutex_unlock(rw_mutex.get());
}
