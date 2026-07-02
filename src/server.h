#pragma once
#include "request.h"
#include "response.h"
#include <atomic>
#include <functional>
#include <optional>
#include <unordered_map>

class Server {
public:
  void listen(int port);
  using Next = std::function<void()>;
  using Middleware = std::function<void(Request &, Response &, Next)>;
  using Handler = std::function<void(Request &, Response &)>;
  void use(Middleware);
  void get(const std::string &path, Handler handler);
  void put(const std::string &path, Handler handler);
  void post(const std::string &path, Handler handler);
  void remove(const std::string &path, Handler handler);
  ~Server();

private:
  int serverFd;
  std::vector<Middleware> middlewares;
  std::unordered_map<std::string, std::unordered_map<std::string, Handler>>
      routes;
  std::optional<Request> parseRequest(int clientFd);
  void dispatch(Request &, Response &);
  void closeServer();
  static std::atomic<bool> serverRunning;
  static void signalHandler(int);
  void handle_headers(const std::string &headers, Request &req);
  void executeMiddlewares(int, Request &, Response &);
};
