#include "server.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/_endian.h>
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
    if (clientFd < 0) {
      std::cerr << "accept failed\n";
      continue;
    }
    std::cout << "Client connected" << '\n';
  }
}

void Server::closeServer() { close(serverFd); }
