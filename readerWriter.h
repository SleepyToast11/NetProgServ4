//
// Created by jerome on 12/9/23.
//

#ifndef NETPROGSERV4_READERWRITER_H
#define NETPROGSERV4_READERWRITER_H

#include <mutex>
#include <memory>

class readerWriter {
public:
    readerWriter();
    readerWriter(readerWriter const &other);
    void rwlock_start_read();
    void rwlock_end_read();
    void rwlock_start_write();
    void rwlock_end_write();
    std::shared_ptr<int> readers;
    std::shared_ptr<pthread_mutex_t> rw_mutex;
    std::shared_ptr<pthread_cond_t> rw_cond;
};


#endif //NETPROGSERV4_READERWRITER_H
