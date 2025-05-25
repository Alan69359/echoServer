/*
 * Author: <configuration>
 * Date: 2024-10-10
 * Copyright Alan69359. All rights reserved.
 */

// PERF: ...
// HACK: ...
// TODO: ...
// NOTE: ...
// FIX: ...
// WARNING: ...

#include "iocpClient.h"
#pragma comment(lib, "ws2_32.lib")

#define TCP_PORT "8080"
#define LOCALHOST "127.0.0.1"

sThread sthread;
WSAEVENT events[1];
Option option;
BOOL isTerminated = FALSE;
BOOL isDisconnected = FALSE;

void debug() { printf("testing\n"); }

BOOL WINAPI HandlerRoutine(DWORD CtrlType) {
  switch (CtrlType) {
  case CTRL_C_EVENT:
  case CTRL_BREAK_EVENT:
  case CTRL_LOGOFF_EVENT:
  case CTRL_SHUTDOWN_EVENT:
  case CTRL_CLOSE_EVENT:
    printf("Closing handles and socket\n");
    SetConsoleCtrlHandler(NULL, TRUE);
    isDisconnected = TRUE;
    for (int i = 0; i < option.ithread; i++) {
      if (sthread.sockets[i] != INVALID_SOCKET) {
        LINGER linger;
        linger = {1, 0};
        setsockopt(sthread.sockets[i], SOL_SOCKET, SO_LINGER, (char *)&linger,
                   sizeof(linger));
        closesocket(sthread.sockets[i]);
        sthread.sockets[i] = INVALID_SOCKET;
        if (sthread.threads[i] != INVALID_HANDLE_VALUE) {
          DWORD resu = WaitForSingleObject(sthread.threads[i], INFINITE);
          if (resu == WAIT_FAILED) {
            printf("WaitForSingleObject() failed with error: %lu\n",
                   GetLastError());
          }
          CloseHandle(sthread.threads[i]);
          sthread.threads[i] = INVALID_HANDLE_VALUE;
        }
      }
    }
    break;
  default:
    return FALSE;
  }
  WSASetEvent(events[0]);
  return TRUE;
}

BOOL send1(int ithread, char *buffer) {
  BOOL isSend = TRUE;
  char *pbuffer = buffer;
  int totalSend = 0;
  int isend = 0;

  while (totalSend < option.sizeofBuffer) {
    int re = send(sthread.sockets[ithread], pbuffer,
                  option.sizeofBuffer - totalSend, 0);
    if (re == SOCKET_ERROR) {
      printf("Send(thread:%d) failed with error:%d\n", ithread,
             WSAGetLastError());
      isSend = FALSE;
      break;
    } else if (!isend) {
      printf("Connection closed\n");
      isSend = FALSE;
      break;
    } else {
      totalSend += isend;
      pbuffer += isend;
    }
  }
  return isSend;
}

BOOL receive1(int ithread, char *buffer) {
  BOOL isReceive = TRUE;
  char *pbuffer = buffer;
  int totalReceive = 0;
  int ireceive = 0;

  while (totalReceive < option.sizeofBuffer) {
    int re = recv(sthread.sockets[ithread], pbuffer,
                  option.sizeofBuffer - totalReceive, 0);
    if (re == SOCKET_ERROR) {
      printf("Receive(thread:%d) failed with error:%d\n", ithread,
             WSAGetLastError());
      isReceive = FALSE;
      break;
    } else if (!ireceive) {
      printf("Connection closed\n");
      isReceive = FALSE;
      break;
    } else {
      totalReceive += ireceive;
      pbuffer += ireceive;
    }
  }
  return isReceive;
}

DWORD WINAPI echoThread(LPVOID para) {
  char *inbuf = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                  option.sizeofBuffer);
  char *outbuf = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                   option.sizeofBuffer);
  int *arg = (int *)para;
  int ithread = *arg;

  printf("Starting thread %d\n", ithread);

  if (inbuf && outbuf) {
    FillMemory(outbuf, option.sizeofBuffer, (BYTE)ithread);

    while (TRUE) {
      int re1 = send1(ithread, outbuf);
      int re2 = receive1(ithread, inbuf);
      if (re1 && re2) {
        if (inbuf[0] == outbuf[0] &&
            inbuf[option.sizeofBuffer - 1] == outbuf[option.sizeofBuffer - 1]) {
          if (option.isVerbose) {
            printf("ack(%d)\n", ithread);
          }
        } else {
          printf("nak(%d) in[0]=%d, out[0]=%d int[%d]=%d out[%d]=%d\n", ithread,
                 inbuf[0], outbuf[0], option.sizeofBuffer - 1,
                 inbuf[option.sizeofBuffer - 1], option.sizeofBuffer - 1,
                 outbuf[option.sizeofBuffer - 1]);
          break;
        }
      } else
        break;
    }
  }
  if (inbuf) {
    HeapFree(GetProcessHeap(), 0, inbuf);
    inbuf = NULL;
  }
  if (outbuf) {
    HeapFree(GetProcessHeap(), 0, outbuf);
    outbuf = NULL;
  }
  return TRUE;
}

void printUsage(Option *option) {
  printf("Usage:\n iocpClient [-h] [-p:%%d] [-v]\n");
  printf("\t-h\t\tDisplay this help\n");
  printf("\t-p [int]\tEndpoint number to use (default:%s)\n", option->port);
  printf(
      "\t-v\t\tVerbose, print an ack when echo server received and verified\n");
  return;
}

int HandlerRoutine(int argc, char *argv[]) {
  option = {"localhost", TCP_PORT, 1, 4096, FALSE};
  int resu = 1;
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      switch (tolower(argv[i][1])) {
      case 'p':
        if (lstrlenA(argv[i]) > 3)
          option.port = &argv[i][3];
        break;
      case 'v':
        option.isVerbose = TRUE;
        break;
      case 'h':
        printUsage(&option);
        resu = 0;
        break;
      default:
        printf("Unknown option flag\n");
        resu = 0;
        break;
      }
    }
  }
  return resu;
}

int main(int argc, char *argv[]) {
  WSADATA wsaData;
  DWORD threadID = 0;
  BOOL isInitialized = FALSE;
  int threads[MAX_THREAD];
  iocpClient *client;

  for (int i = 0; i < MAX_THREAD; i++) {
    sthread.sockets[i] = INVALID_SOCKET;
    sthread.threads[i] = INVALID_HANDLE_VALUE;
    threads[i] = 0;
  }

  events[0] = WSA_INVALID_EVENT;

  if (!HandlerRoutine(argc, argv))
    return 1;

  int resu1 = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (resu1) {
    printf("WSAStartup failed: %d\n", resu1);
    SetConsoleCtrlHandler(HandlerRoutine, FALSE);
    return 1;
  }

  events[0] = WSACreateEvent();
  if (events[0] == WSA_INVALID_EVENT) {
    printf("WSACreateEvent() failed with error: %d\n", WSAGetLastError());
    WSACleanup();
    return 1;
  }

  if (!SetConsoleCtrlHandler(HandlerRoutine, TRUE)) {
    printf("SetConsoleCtrlHandler() failed with error: %lu\n", GetLastError());
    if (events[0] != WSA_INVALID_EVENT) {
      WSACloseEvent(events[0]);
      events[0] = WSA_INVALID_EVENT;
    }
    WSACleanup();
    return 1;
  }

  for (int i = 0; i < option.ithread && isInitialized; i++) {
    if (isTerminated)
      break;
    else {
      client = new iocpClient();
      client->translation(option.host, option.port, IPPROTO_TCP, 0);
      client->createSocket();
      client->bindSocket();
      client->listenSocket();
      client->setSocketOption();
      threads[i] = i;
      sthread.threads[i] =
          CreateThread(NULL, 0, echoThread, (LPVOID)&threads[i], 0, &threadID);
      if (!sthread.threads[i]) {
        printf("CreateThread(id %d) failed with error: %lu\n", i,
               GetLastError());
        isInitialized = FALSE;
        break;
      }
    }
  }

  if (isInitialized) {
    DWORD resu =
        WaitForMultipleObjects(option.ithread, sthread.threads, TRUE, INFINITE);
    if (resu == WAIT_FAILED) {
      printf("WaitForMultipleObjects() failed with error: %lu\n",
             GetLastError());
    }
  }
  if (!GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0)) {
    printf("pid: %lu\n", GetCurrentThreadId());
    printf("GenerateConsoleCtrlEvent() failed with error: %lu\n",
           GetLastError());
  }
  Sleep(4000);
  DWORD resu2 = WSAWaitForMultipleEvents(1, events, TRUE, WSA_INFINITE, FALSE);
  if (resu2 == WSA_WAIT_FAILED)
    printf("WSAWaitForMultipleEvents() failed with error: %lu\n",
           GetLastError());
  if (events[0] != WSA_INVALID_EVENT) {
    WSACloseEvent(events[0]);
    events[0] = WSA_INVALID_EVENT;
  }
  WSACleanup();
  printf("breakpoint\n");
  SetConsoleCtrlHandler(HandlerRoutine, FALSE);
  SetConsoleCtrlHandler(NULL, FALSE);
  return 0;
}
