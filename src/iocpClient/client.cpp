/*
 * Author: <configuration>
 * Date: 2025-02-19
 * Copyright Alan69359. All rights reserved.
 */

// PERF: ...
// HACK: ...
// TODO: ...
// NOTE: ...
// FIX: ...
// WARNING: ...

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

const int PORT = 8080;
const int BUFFER_SIZE = 1024;

int main() {
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    std::cerr << "WSAStartup failed\n";
    return 1;
  }

  SOCKET clientSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0,
                                  WSA_FLAG_OVERLAPPED);
  if (clientSocket == INVALID_SOCKET) {
    std::cerr << "WSASocket failed: " << WSAGetLastError() << std::endl;
    WSACleanup();
    return 1;
  }

  sockaddr_in serverAddr{};
  serverAddr.sin_family = AF_INET;
  inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
  serverAddr.sin_port = htons(PORT);

  if (connect(clientSocket, reinterpret_cast<sockaddr *>(&serverAddr),
              sizeof(serverAddr)) == SOCKET_ERROR) {
    std::cerr << "Connect failed: " << WSAGetLastError() << std::endl;
    closesocket(clientSocket);
    WSACleanup();
    return 1;
  }

  // Send HTTP GET request
  const char *request = "GET / HTTP/1.1, Host: localhost\r\n\r\n";
  WSABUF wsaBuf;
  wsaBuf.buf = const_cast<char *>(request);
  wsaBuf.len = static_cast<ULONG>(strlen(request));

  DWORD bytesSent;
  WSASend(clientSocket, &wsaBuf, 1, &bytesSent, 0, nullptr, nullptr);

  // Receive response
  char buffer[BUFFER_SIZE];
  wsaBuf.buf = buffer;
  wsaBuf.len = BUFFER_SIZE;

  DWORD bytesReceived, flags = 0;
  WSARecv(clientSocket, &wsaBuf, 1, &bytesReceived, &flags, nullptr, nullptr);

  std::cout << "Received response:\n" << buffer << std::endl;

  closesocket(clientSocket);
  WSACleanup();
  return 0;
}
