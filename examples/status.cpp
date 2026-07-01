#include "../src/request.h"
#include "../src/response.h"
#include "../src/server.h"
#include <iostream>

int main() {
  Server app;

  app.get("/success", [](Request &req, Response &res) {
    res.status(200).send("Success!");
  });

  app.get("/notfound", [](Request &req, Response &res) {
    res.status(404).send("Page not found");
  });

  app.get("/error", [](Request &req, Response &res) {
    res.status(500).send("Internal server error");
  });

  std::cout << "Server running on port 3000\n";
  app.listen(3000);
}
