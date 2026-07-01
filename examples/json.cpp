#include "../src/request.h"
#include "../src/response.h"
#include "../src/server.h"
#include <iostream>

int main() {
  Server app;

  app.get("/api/user", [](Request &req, Response &res) {
    // Simple JSON response
    std::string userJson = R"({"name": "Ariadne", "role": "admin"})";
    res.json(userJson);
  });

  std::cout << "Server running on port 3000\n";
  app.listen(3000);
}
