cmake_minimum_required(VERSION 3.25)
project(NetProgServ4)

set(CMAKE_CXX_STANDARD 23)

add_executable(NetProgServ4 main.cpp SyncServ.cpp SyncServ.h tcp-utils.cpp tcp-utils.h Server.cpp Server.h FileServ.cpp FileServ.h ShellServ.cpp ShellServ.h Worker.cpp Worker.h FileAccesser.cpp FileAccesser.h FileSupervisor.cpp FileSupervisor.h readerWriter.cpp readerWriter.h SyncFileAccessor.cpp SyncFileAccessor.h)
