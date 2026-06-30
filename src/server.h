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
  std::unordered_map<std::string, std::unordered_map<std::string, Handler>>
      routes;
  Request parseRequest(int clientFd);
  void dispatch(Request &, Response &);
  void closeServer();
  static std::atomic<bool> serverRunning;
  static void signalHandler(int);
  void handle_headers(const std::string &headers, Request &req);

public:
  void listen(int port);
  void get(const std::string &path, Handler handler);
  void put(const std::string &path, Handler handler);
  void post(const std::string &path, Handler handler);
  void remove(const std::string &path, Handler handler);
  ~Server();
};
