#pragma once

#include "../../lib/I_O_completion_port.h"
#include "../../lib/Winsock_server.cpp"
#include <vector>
#include <iostream>

const int MAX_WORKER_THREAD = 16;

class iocpServer : public Server {
private:
  HANDLE completionPort;
  CRITICAL_SECTION criticalSection;
  std::vector<HANDLE> handles;// handles of thread
  bool state=0;// 0: not running, 1: running, 2: restarting

public:
  iocpServer() : completionPort(INVALID_HANDLE_VALUE), state(1) {
    for(int i = 0; i < MAX_WORKER_THREAD; i++) {
      handles[i] = INVALID_HANDLE_VALUE;
    }
    
    
  }
  ~iocpServer() { exit1(); }

  HANDLE getCompletionPort() { return completionPort; }
  CRITICAL_SECTION& getCriticalSection() { return criticalSection; }
  int getState() { return state; }

  // create IO completion port
  void create2(HANDLE fileHandle, HANDLE existingPort,
                             ULONG_PTR completionKey, DWORD numberOfThreads);
  
  // handle client connection
  void handler1(SOCKET clientSocket);
  // process IO operation
  void handler2(cIOOperation *operation, DWORD bytesTransferred);

  // initialize worker threads
  void initialize2();
  // cleanup worker threads
  void cleanup2();

  void exit1() override{
    isRunning = false;
    cleanup2();
    cleanup1();
    WSACleanup();
  };
  void cleanup1() override;
  DWORD WINAPI WorkerThread(LPVOID context);
};int enterCriticalSection(CRITICAL_SECTION section);

BOOL WINAPI HandlerRoutine(DWORD event);
