#include "../../lib/I_O_completion_port.h"
#include "../../lib/Winsock_server.cpp"

#define MAX_THREAD 2

struct sThread {
  HANDLE threads[MAX_THREAD];
  SOCKET sockets[MAX_THREAD];
};

struct Option {
  const char *host;
  const char *port;
  int ithread;
  int sizeofBuffer;
  BOOL isVerbose;
};

class iocpClient : public Server {
public:
  iocpClient() = default;
  iocpClient(iocpClient &&client);
  ~iocpClient();
};

BOOL WINAPI HandlerRoutine(DWORD fdwCtrlType);
BOOL send1(int ithread, char *buffer);
BOOL receive1(int ithread, char *buffer);
DWORD WINAPI echoThread(LPVOID para);
