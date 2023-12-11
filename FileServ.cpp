#include "FileServ.h"
#include "Server.h"

int FileServ::getPort(){
    return FileServ::port;
}

void FileServ::setPort(int port) {
    FileServ::port = port;
}

void FileServ::setFileSupervisor(std::shared_ptr<FileSupervisor> fileSupervisor) {
    files = fileSupervisor;
};