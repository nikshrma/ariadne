#include "server.h"
#include "request.h"
#include "response.h"
#include <arpa/inet.h>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/_endian.h>
#include <sys/_types/_ssize_t.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <unistd.h>

std::atomic<bool> Server::serverRunning{true};
void Server::signalHandler(int signal) { serverRunning = false; }

void Server::listen(int port) {
  struct sigaction sa{};
  sa.sa_flags = 0;
  sa.sa_handler = Server::signalHandler;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, nullptr);
  sigaction(SIGTERM, &sa, nullptr);
  serverFd = socket(AF_INET, SOCK_STREAM, 0);
  if (serverFd < 0) {
    std::cerr << "Failed to create socket: " << std::strerror(errno) << '\n';
    return;
  }
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);
  int opt = 1;
  setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (bind(serverFd, (sockaddr *)&address, sizeof(address)) < 0) {
    std::cerr << "Failed to bind: " << std::strerror(errno) << '\n';
    return;
  }
  if (::listen(serverFd, SOMAXCONN) < 0) {
    std::cerr << "Failed to listen: " << std::strerror(errno) << '\n';
    return;
  }
  std::cout << "listening on port -> " << port << '\n';

  while (serverRunning) {
    int clientFd = accept(serverFd, nullptr, nullptr);
    if (clientFd == -1) {
      if (errno == EINTR) {
        continue;
      }
      perror("accept");
      break;
    }
    Request req = parseRequest(clientFd);
    Response res(clientFd);
    dispatch(req, res);
    close(clientFd);
  }
}

void Server::closeServer() { close(serverFd); }

void Server::get(std::string path, Handler handler) {
  getRoutes[path] = handler;
}

Request Server::parseRequest(int clientFd) {
  char buffer[4096];

  ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);
  if (bytesRead <= 0) {
    return {};
  }

  std::string request(buffer, bytesRead);

  std::istringstream stream(request);
  std::string method;
  std::string path;
  std::string version;

  stream >> method >> path >> version;
  std::cout << "method:" << method << '\n'
            << "path:" << path << '\n'
            << "version:" << version << '\n';
  Request req;
  req.method = method;
  req.path = path;
  return req;
}

void Server::dispatch(Request &req, Response &res) {
  auto route = getRoutes.find(req.path);
  if (route != getRoutes.end()) {
    route->second(req, res);
  } else
    res.send("404");
}
Server::~Server() { closeServer(); }
