#pragma once
#include "request.h"
#include "response.h"
#include <atomic>
#include <functional>
#include <unordered_map>

class Server {
private:
  int serverFd;
  using Handler = std::function<void(Request &, Response &)>;
  std::unordered_map<std::string, Handler> getRoutes;
  Request parseRequest(int clientFd);
  void dispatch(Request &, Response &);
  void closeServer();
  static std::atomic<bool> serverRunning;
  static void signalHandler(int);

public:
  void listen(int port);
  void get(std::string path, Handler handler);
  ~Server();
};
