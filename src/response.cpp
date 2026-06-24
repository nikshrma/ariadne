
#include "response.h"
#include <sys/socket.h>
void Response::send(const std::string &body) {
  std::string response = "HTTP/1.1 200 OK\r\n"
                         "Content-Length: " +
                         std::to_string(body.size()) + "\r\n\r\n" + body;
  ::send(clientFd, response.c_str(), response.size(), 0);
}
Response::Response(int clientFd) : clientFd(clientFd) {};
