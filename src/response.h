#pragma once
#include <string>

class Response {
private:
  int clientFd;
  int statusCode;
  std::string getStatusPhrase();

public:
  Response(int clientFd);
  void send(const std::string &body);
  Response &status(int st);
};
