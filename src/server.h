#pragma once

class Server {
private:
  int serverFd;

public:
  void listen(int port);
  void closeServer();
};
