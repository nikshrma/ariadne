#pragma once
#include "request.h"
#include "response.h"
#include <functional>
#include <unordered_map>

class Server {
private:
  int serverFd;
  using Handler = std::function<void(Request &, Response &)>;
  std::unordered_map<std::string, Handler> getRoutes;

public:
  void listen(int port);
  void closeServer();
  void get(std::string path, Handler handler);
};
