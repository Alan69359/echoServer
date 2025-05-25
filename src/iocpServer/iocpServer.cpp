/*
 * Author: 
 * Date: 2024-09-26
 * Modified: 2024-09-26 16:52:20
 * Copyright Alan69359. All rights reserved.
 */

// PERF: ...
// HACK: ...
// TODO: ...
// NOTE: ...
// FIX: expand diagnostic: lua vim.diagnostic.open_float()
// WARNING: ...

#include "iocpServer.h"
#include "../../lib/Thread_Pool.cpp"
#include <WinSock2.h>
#include <mutex>
#include <processthreadsapi.h>
#include <stdio.h>

#define LOCALHOST "127.0.0.1"
#define TCP_PORT "8080"

int isRunning = 0;
int ithread = 0;                      // number of thread
int isAdd = 0;                     // add current client to the list of active clients connecions?
HANDLE handle = INVALID_HANDLE_VALUE; // handle of IOCP
iocpServer *server = NULL;
SOCKET socket1;       // socket of server
SOCKET socket2;       // socket of client
cSOCKET *head = nullptr; // head of a linked list of sockets
DWORD byte1;// number of byte received
DWORD flag;

class CriticalSectionGuard {
private:
  CRITICAL_SECTION &criticalSection;
public:
CriticalSectionGuard(CRITICAL_SECTION &section) : criticalSection(section) {
    EnterCriticalSection(&criticalSection);
  }
  ~CriticalSectionGuard() {
    LeaveCriticalSection(&criticalSection);
  }
  CriticalSectionGuard(const CriticalSectionGuard&) = delete;
  CriticalSectionGuard& operator=(const CriticalSectionGuard&) = delete;
};

void iocpServer::cleanup2() {
  for (auto &thread : workerThreads) {
    if (thread != NULL) {
      WaitForSingleObject(thread, INFINITE);
      CloseHandle(thread);
    }
  }
  workerThreads.clear();
}

void iocpServer::handler2(cIOOperation *operation, DWORD bytesTransferred) {
  if (bytesTransferred == 0) {
    printf("Client disconnected\n");
    closesocket(operation->socket);
    delete operation;
    return;
  }

  // Process received data
  printf("Received %lu bytes from client\n", bytesTransferred);

  // Echo back the received data
  WSABUF buffer;
  buffer.buf = operation->buffers;
  buffer.len = bytesTransferred;

  DWORD bytesSent;
  DWORD flags = 0;
  if (WSASend(operation->socket, &buffer, 1, &bytesSent, flags,
              &operation->overlapped, nullptr) == SOCKET_ERROR) {
    if (WSAGetLastError() != ERROR_IO_PENDING) {
      printf("WSASend failed with error: %d\n", WSAGetLastError());
      closesocket(operation->socket);
      delete operation;
    }
  }
}

DWORD WINAPI iocpServer::WorkerThread(LPVOID context) {
  auto server = static_cast<iocpServer *>(context);
  DWORD bytesTransferred;
  ULONG_PTR completionKey;
  LPOVERLAPPED overlapped;

  while (server->isRunning) {
    BOOL result =
        GetQueuedCompletionStatus(server->completionPort, &bytesTransferred,
                                  &completionKey, &overlapped, INFINITE);

    if (!result) {
      printf("GetQueuedCompletionStatus failed with error: %lu\n",
             GetLastError());
      continue;
    }

    auto operation = static_cast<cIOOperation *>(overlapped);
    server->handler2(operation, bytesTransferred);
  }

  return 0;
}

void iocpServer::cleanup() {
  if (completionPort != NULL) {
    CloseHandle(completionPort);
    completionPort = NULL;
  }
  Server::cleanup();
}

void iocpServer::create2(HANDLE fileHandle, HANDLE existingPort,
                                       ULONG_PTR completionKey,
                                       DWORD numberOfThreads) {
  completionPort = CreateIoCompletionPort(fileHandle, existingPort,
                                          completionKey, numberOfThreads);
  if (completionPort == NULL) {
    throw std::runtime_error("CreateIoCompletionPort() failed");
  }
}

void postAccept(iocpServer *server) {
  SOCKET sock =
      WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
  if (sock == INVALID_SOCKET)
    return;

  cIOOperation *cIOoper = new cIOOperation();
  if (cIOoper == nullptr) {
    closesocket(sock);
    return;
  }

  ZeroMemory(cIOoper, sizeof(cIOOperation));
  cIOoper->socket = sock;
  cIOoper->buffer.buf = cIOoper->buffers;
  cIOoper->buffer.len = BUFSIZ;
  cIOoper->operation = eIOOperation::Accept;

  DWORD byte1 = 0; // byte received
  while (AcceptEx(server->getSocket(), sock, cIOoper->buffer.buf, 0,
                  sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &byte1,
                  (LPOVERLAPPED)cIOoper) == false) {
    if (WSAGetLastError() == WSA_IO_PENDING)
      break;
  }
}

void postRead(iocpServer *server) {}

void disconnect(cSOCKET *csock) {
  // TODO: isgraceful disconnection
  CriticalSectionGuard guard(server->getCriticalSection());
  if (csock != NULL)
    return;

  closesocket(csock->getSocket());
}

int main(int argc, char *argv[]){
  SYSTEM_INFO info;
  WSADATA data;
  iocpServer *server=new iocpServer();
  byte1=0;
  flag=0;

  GetSystemInfo(&info);
  g_dwThreadCount = systemInfo.dwNumberOfProcessors * 2;

  server->initialize1(&data);

  while(server->getState() == 1) {
    try
    {
      server->create2(INVALID_HANDLE_VALUE, NULL, 0, 0);
      for (DWORD i = 0; i < info.dwNumberOfProcessors*2; i++)
      {
        HANDLE hand=INVALID_HANDLE_VALUE;
        DWORD ID=0;
        hand = CreateThread(NULL, 0, WorkerThread, server->getCompletionPort(), 0, &ID);
        if(hand == NULL) {
          throw std::runtime_error("CreateThread() failed");
        }
      }

      server->create1(LOCALHOST, TCP_PORT, AF_INET, SOCK_STREAM, IPPROTO_IP, AI_PASSIVE, WSA_FLAG_OVERLAPPED);
      server->bind1();
      server->listen1();
      int resu1=0;
      server->set1(server->getSocket(),SOL_SOCKET,SO_SNDBUF,(char *)&resu1,sizeof(resu1));
      server->initialize2();
      
    }
    catch(const std::exception& e)
    {
      std::cerr << "Error: "<<GetLastError()<<std::endl;
    }

    cSOCKET *csock=new cSOCKET();
    while(1){
      try
      {
        sockaddr_in6 addr;// address of client
        socket2=server->accept2(addr, NULL);

        // UpdateCompletionPort() start
        cSOCKET *csock;
        CriticalSectionGuard guard(server->getCriticalSection());
        OVERLAPPED *over = new OVERLAPPED(0,0,0,0,NULL);
        WSABUF *buff=new WSABUF();
        cIOOperation *oper = new cIOOperation();
        oper->overlapped = *over;
        oper->buffer.buf = oper->buffers;
        oper->buffer.len = sizeof(oper->buffers);
        oper->operation=eIOOperation::Accept;
        oper->next=NULL;
        csock->setSocket(socket2);
        csock->setOperation(oper);
        csock->prev=csock->next=NULL;
        ZeroMemory(oper->buffer.buf, sizeof(oper->buffer.buf));
      }
      catch(const std::exception& e)
      {
        delete csock;
        std::cerr<< "Error: "<<GetLastError()<<std::endl;
      }
      
    }

    if(csock==NULL) {
      throw std::runtime_error("Failed to update completion port");
    }

    try
    {
      server->create2((HANDLE)socket2, server->getCompletionPort(), (DWORD_PTR)csock, 0);
    }
    catch(const std::exception& e)
    {
      delete csock;
      std::cerr << "Error: "<<GetLastError()<<std::endl;
    }
    
    if(isAdd==1){
      CriticalSectionGuard guard(server->getCriticalSection());
      if(head==nullptr){
        head=csock;
        head->next=nullptr;
        head->prev=nullptr;
      } else {
        head->prev=csock;
        csock->prev=nullptr;
        csock->next=head;
        head=csock;
      }
    }
    // UpdateCompletionPort() end

    if(csock==NULL){
      throw std::runtime_error("Failed to update completion port");
    }

    server->receive2(socket2, &(csock->getOperation()->buffer), 1, &byte1, &flag, &(csock->getOperation()->overlapped), NULL);
  }
  
}