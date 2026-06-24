#pragma once
#include <string>

class Response {
private:
  int clientFd;

public:
  Response(int clientFd);
  void send(const std::string &body);
};
