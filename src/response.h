#pragma once
#include <string>
#include <unordered_map>

class Response {
private:
  int clientFd;
  int statusCode;
  std::string getStatusPhrase() const;
  std::unordered_map<std::string, std::string> headers;

public:
  Response(int clientFd);
  void send(const std::string &body);
  Response &status(int st);
  Response &set(const std::string &key, const std::string &value);
  void json(const std::string &body);
};
