#include "request.h"
#include "response.h"
#include "server.h"
#include <iostream>

int main() {
  Server app;

  app.use([](Request &req, Response &res, Server::Next next) {
    std::cout << "[Logger] " << req.method << " " << req.path << '\n';
    next();
  });

  app.use([](Request &req, Response &res, Server::Next next) {
    res.status(401).send("Unauthorized");
  });

  app.use([](Request &req, Response &res, Server::Next next) {
    std::cout << "[Middleware 3]\n";
    next();
  });

  app.get("/", [](Request &req, Response &res) {
    std::cout << "[Handler]\n";
    res.send("Home");
  });

  app.get("/users/:id", [](Request &req, Response &res) {
    std::cout << "[Handler]\n";
    res.send("User ID = " + req.params["id"]);
  });

  app.post("/login", [](Request &req, Response &res) {
    std::cout << "[Handler]\n";
    res.status(201).json(R"({"success":true})");
  });

  app.listen(3000);
}
