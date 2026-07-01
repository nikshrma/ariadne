#include "server.h"
#include "request.h"
#include "response.h"
#include "threadpool.h"
#include <arpa/inet.h>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/signal.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <utility>

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
  ThreadPool pool(std::thread::hardware_concurrency());

  while (serverRunning) {
    int clientFd = accept(serverFd, nullptr, nullptr);
    if (clientFd == -1) {
      if (errno == EINTR) {
        continue;
      }
      perror("accept");
      break;
    }
    pool.enqueue([this, clientFd] {
      Request req = parseRequest(clientFd);
      Response res(clientFd);
      executeMiddlewares(0, req, res);
      close(clientFd);
    });
  }
}

void Server::closeServer() { close(serverFd); }

Request Server::parseRequest(int clientFd) {
  char buffer[4096];
  size_t delimiter_pos = std::string::npos;
  std::string request;
  while (delimiter_pos == std::string::npos) {
    ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);
    if (bytesRead <= 0) {
      return {};
    }
    request.append(buffer, bytesRead);
    delimiter_pos = request.find("\r\n\r\n");
  }
  std::string headers = request.substr(0, delimiter_pos);
  std::string body_leftover = request.substr(delimiter_pos + 4);
  Request req;
  handle_headers(headers, req);
  req.body = body_leftover;
  auto it = req.headers.find("Content-Length");
  if (it != req.headers.end()) {
    size_t contentLength = std::stoul(it->second);
    while (contentLength > req.body.size()) {
      ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);
      if (bytesRead <= 0) {
        return {};
      }
      req.body.append(buffer, bytesRead);
    }
  }
  return req;
}
void Server::use(Middleware middleware) {
  middlewares.push_back(std::move(middleware));
}
void Server::executeMiddlewares(int index, Request &req, Response &res) {
  if (index == middlewares.size()) {
    dispatch(req, res);
    return;
  } else
    middlewares[index](
        req, res, [&, index]() { executeMiddlewares(index + 1, req, res); });
}
inline std::string_view to_sv(auto &&rng) {
  return std::string_view(rng.begin(), rng.end());
}

bool matchRoutes(const std::string &pattern, const std::string &path,
                 Request &req, Response &res) {

  auto patternSegments = pattern | std::views::split('/');
  auto pathSegments = path | std::views::split('/');
  auto it1 = patternSegments.begin();
  auto it2 = pathSegments.begin();

  std::unordered_map<std::string, std::string> tempParams;

  while (it1 != patternSegments.end() && it2 != pathSegments.end()) {
    auto sv1 = to_sv(*it1);
    auto sv2 = to_sv(*it2);

    if (sv1.empty()) {
      ++it1;
      continue;
    }
    if (sv2.empty()) {
      ++it2;
      continue;
    }

    if (sv1 == sv2) {
      ++it1;
      ++it2;
      continue;
    } else if (sv1[0] == ':') {
      std::string paramName = std::string(sv1.substr(1));
      tempParams[paramName] = std::string(sv2);
    } else {
      return false;
    }

    ++it1;
    ++it2;
  }

  while (it1 != patternSegments.end() && to_sv(*it1).empty()) {
    ++it1;
  }
  while (it2 != pathSegments.end() && to_sv(*it2).empty()) {
    ++it2;
  }

  if (it1 != patternSegments.end() || it2 != pathSegments.end()) {
    return false;
  }

  for (const auto &[key, val] : tempParams) {
    req.params[key] = val;
  }

  return true;
}

void Server::dispatch(Request &req, Response &res) {
  auto methodIt = routes.find(req.method);
  if (methodIt == routes.end()) {
    res.status(405).send("Method Not Allowed");
    return;
  }
  for (auto &[pattern, handler] : methodIt->second) {
    if (matchRoutes(pattern, req.path, req, res)) {
      handler(req, res);
      return;
    }
  }
  res.status(405).send("Not Found");
}

void Server::get(const std::string &path, Handler handler) {
  routes["GET"][path] = handler;
}
void Server::post(const std::string &path, Handler handler) {
  routes["POST"][path] = handler;
}
void Server::put(const std::string &path, Handler handler) {
  routes["PUT"][path] = handler;
}
void Server::remove(const std::string &path, Handler handler) {
  routes["DELETE"][path] = handler;
}
void Server::handle_headers(const std::string &headers, Request &req) {
  std::istringstream stream(headers);
  std::string line;
  std::getline(stream, line);
  std::istringstream requestLine(line);
  requestLine >> req.method >> req.path >> req.version;
  std::cout << "method:" << req.method << '\n'
            << "path:" << req.path << '\n'
            << "version:" << req.version << '\n';
  while (std::getline(stream, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    auto colon = line.find(":");
    if (colon == std::string::npos) {
      continue;
    }
    std::string key = line.substr(0, colon);
    std::string value = line.substr(colon + 1);
    while (!value.empty() && value.front() == ' ')
      value.erase(0, 1);
    req.headers[key] = value;
  }
}
Server::~Server() { closeServer(); }
