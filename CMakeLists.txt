cmake_minimum_required(VERSION 3.29)

set(DCMAKE_EXPORT_COMPILE_COMMANDS ON)

project(iocp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SRC1
  # "${PROJECT_SOURCE_DIR}/lib/I_O_completion_port.h"
  # "${PROJECT_SOURCE_DIR}/lib/Winsock_server.cpp"
  "${PROJECT_SOURCE_DIR}/src/iocpServer/*.h"
  "${PROJECT_SOURCE_DIR}/src/iocpServer/*.cpp"
)

add_executable(iocpServer ${SRC1})

file(GLOB SRC2
  # "${PROJECT_SOURCE_DIR}/lib/I_O_completion_port.h"
  # "${PROJECT_SOURCE_DIR}/lib/Winsock_server.cpp"
  "${PROJECT_SOURCE_DIR}/src/iocpClient/*.h"
  "${PROJECT_SOURCE_DIR}/src/iocpClient/*.cpp"
)

add_executable(iocpClient ${SRC2})
