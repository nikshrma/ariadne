
#include "response.h"
#include <stdexcept>
#include <string>
#include <sys/socket.h>
Response &Response::status(int st) {
  if (st < 100 || st > 599) {
    throw std::invalid_argument("HTTP status code is invalid");
  }
  statusCode = st;
  return *this;
}
Response &Response::set(const std::string &key, const std::string &value) {
  std::string trimmed = value;

  while (!trimmed.empty() && trimmed.front() == ' ')
    trimmed.erase(0, 1);

  headers[key] = trimmed;
  headers[key] = trimmed;
  return *this;
}
void Response::send(const std::string &body) {
  // first line
  std::string response = "HTTP/1.1 " + std::to_string(statusCode) + " " +
                         getStatusPhrase() + "\r\n";
  // headers
  for (const auto &[key, value] : headers) {
    response = response + key + ": " + value + "\r\n";
  }
  // last header
  response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
  // headers end + body
  response += "\r\n" + body;
  ::send(clientFd, response.c_str(), response.size(), 0);
}
void Response::json(const std::string &body) {
  if (headers.find("Content-Type") == headers.end())
    set("Content-Type", "application/json");

  send(body);
  send(body);
}
Response::Response(int clientFd) : clientFd(clientFd), statusCode(200) {}
std::string Response::getStatusPhrase() const {
  switch (statusCode) {
  case 200:
    return "OK";
  case 201:
    return "Created";
  case 204:
    return "No Content";
  case 301:
    return "Moved Permanently";
  case 302:
    return "Found";
  case 400:
    return "Bad Request";
  case 401:
    return "Unauthorized";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 405:
    return "Method Not Allowed";
  case 409:
    return "Conflict";
  case 500:
    return "Internal Server Error";
  case 501:
    return "Not Implemented";
  case 502:
    return "Bad Gateway";
  case 503:
    return "Service Unavailable";
  default:
    return "Unknown";
  }
}
