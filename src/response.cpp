
#include "response.h"
#include <iostream>
#include <string>
#include <sys/socket.h>
Response &Response::status(int st) {
  if (st < 100 || st > 599) {
    std::cerr << "Invalid statusCode" << '\n';
  }
  statusCode = st;
  return *this;
}
void Response::send(const std::string &body) {
  std::string response = "HTTP/1.1 " + std::to_string(statusCode) +
                         " OK\r\n"
                         "Content-Length: " +
                         std::to_string(body.size()) + "\r\n\r\n" + body;
  ::send(clientFd, response.c_str(), response.size(), 0);
}
Response::Response(int clientFd) : clientFd(clientFd), statusCode(200) {}
