#include "server.h"
#include "request.h"
#include "response.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <sys/_endian.h>
#include <sys/_types/_ssize_t.h>
#include <sys/socket.h>
#include <unistd.h>

void Server::listen(int port) {
  serverFd = socket(AF_INET, SOCK_STREAM, 0);
  if (serverFd < 0) {
    std::cerr << "Failed to create socket: " << std::strerror(errno) << '\n';
    return;
  }
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  if (bind(serverFd, (sockaddr *)&address, sizeof(address)) < 0) {
    std::cerr << "Failed to bind: " << std::strerror(errno) << '\n';
    return;
  }
  if (::listen(serverFd, SOMAXCONN) < 0) {
    std::cerr << "Failed to listen: " << std::strerror(errno) << '\n';
    return;
  }
  std::cout << "listening on port -> " << port << '\n';

  while (true) {
    int clientFd = accept(serverFd, nullptr, nullptr);
    char buffer[4096];

    ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);

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
    Response res(clientFd);
    if (getRoutes.find(req.path) != getRoutes.end()) {
      getRoutes[req.path](req, res);
    } else
      res.send("404");
    close(clientFd);
  }
}

void Server::closeServer() { close(serverFd); }
void Server::get(std::string path, Handler handler) {
  getRoutes[path] = handler;
}
