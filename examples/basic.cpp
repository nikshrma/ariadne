#include "../src/request.h"
#include "../src/response.h"
#include "../src/server.h"
#include <iostream>

void loggerMiddleware(Request &req, Response &res, Server::Next next) {
  std::cout << "Received " << req.method << " request to " << req.path << '\n';
  next();
}

int main() {
  Server app;

  app.use(loggerMiddleware);

  app.get("/", [](Request &req, Response &res) {
    res.send("Hello from GET!");
  });

  app.post("/submit", [](Request &req, Response &res) {
    res.send("Received POST data: " + req.body);
  });

  std::cout << "Server running on port 3000\n";
  app.listen(3000);
}
