#include "../../src/request.h"
#include "../../src/response.h"
#include "../../src/server.h"
#include <iostream>

void loggerMiddleware(Request &req, Response &res, Server::Next next) {
  // std::cout << "Received " << req.method << " request to " << req.path <<
  // '\n';
  next();
}

int main() {
  Server app;

  app.use(loggerMiddleware);

  app.get("/", [](Request &req, Response &res) {
    res.status(200).send("Hello from GET!");
  });

  app.post("/submit", [](Request &req, Response &res) {
    res.status(200).send("Received POST data: " + req.body);
  });

  app.get("/api/user", [](Request &req, Response &res) {
    res.status(200).json(R"({"name": "Ariadne", "role": "admin"})");
  });

  app.get("/error", [](Request &req, Response &res) {
    res.status(500).send("Internal server error");
  });

  std::cout << "Ariadne benchmark server running on port 3000\n";
  app.listen(3000);
}
