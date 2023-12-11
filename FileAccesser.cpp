//
// Created by jerome on 12/5/23.
//

#include "FileAccesser.h"
#include <utility>
#include <fcntl.h>
#include "tcp-utils.h"
#include <sys/wait.h>
#include "FileSupervisor.h"

//quick macro to verify if file is opened when it should be
#define CHECKOPEN \
if(!open)      \
return Message(ERR, ENOENT, "File not opened");

Message FileAccessor::rw_open(){
    rwlock_start_write();
    Message result = i_open();
    rwlock_end_write();
    return result;
}

Message FileAccessor::rw_close(){
    CHECKOPEN
    rwlock_start_write();
    Message result = i_close();
    rwlock_end_write();
    return result;
}

Message FileAccessor::write_w(const char* stuff, size_t stuff_length){
    CHECKOPEN
    rwlock_start_write();
    Message result = i_write(stuff, stuff_length);
    rwlock_end_write();
    return result;
}

Message FileAccessor::read_w(int bytes){
    CHECKOPEN
    rwlock_start_read();
    Message result = i_read(bytes);
    rwlock_end_read();
    return result;
}

Message FileAccessor::seek_w(off_t offset){
    CHECKOPEN
    rwlock_start_write();
    Message result = i_seek(offset);
    rwlock_end_write();
    return result;
}

FileAccessor::FileAccessor(std::string fileName){
    this->fileName = std::move(fileName);
}

Message FileAccessor::i_open(){
    int fdTmp;
    if(opened)
        return {ERR, fd, "file already opened, please use the supplied identifier"};

    fdTmp = open(fileName.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR); ///Check for / PLEASE

    if ( fdTmp < -1 )
        return {FAIL, errno, ErrStr()};

    fd = fdTmp;
    FileSupervisor::setFileDescriptor(fileName, fd);
    opened = true;
    owners++;
    return {OK, fdTmp, "File was successfully opened"};
}

Message FileAccessor::i_close(){
    owners--;
    if ( owners > 0)
        return {OK, 0, "file closed"};

    owners = 0;
    opened = false; //whether or not close works, better make the next call fail and make client reopen
    int closed = close(fd);
    if (closed < 0) {
        return {FAIL,errno, ErrStr()};
    }
    return {OK, 0, "file closed"};

}

Message FileAccessor::i_write(const char* stuff, size_t stuff_length) const{
    CHECKOPEN
    int pid = fork();
    if(pid != 0)
        return {OK, 0, "Write will be completed"};
    write(fd, stuff, stuff_length);
    exit(0);
} //write will always return non-blocking


Message FileAccessor::i_read(int bytes) const{
    std::string buf;
    if (bytes > MAX_LEN)
        bytes = MAX_LEN;
    buf.resize(bytes);
    int numRead = read(fd, &buf[0], buf.size());
    if(numRead < 0)
        return {FAIL, errno, ErrStr()};
    return {OK, numRead, buf};
}

Message FileAccessor::i_seek(off_t offset) const{
    int result;
    result = lseek(fd, offset, SEEK_CUR);
    if (result == -1)
        return {FAIL, errno, ErrStr()};
    return {OK, 0, "The cursor was moved successfully"};
}

FileAccessor::FileAccessor( FileAccessor const &accessor)  : readerWriter(accessor) {
    this->fileName = accessor.fileName;
}
