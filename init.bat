@echo off

g++ -o a ./src/iocpServer/iocpServer.cpp -lws2_32
g++ -o b ./src/iocpClient/client.cpp -lws2_32

start a.exe

start cmd /k "b.exe & pause & exit"
