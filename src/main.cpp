#include "request.h"
#include "response.h"
#include "server.h"
#include <iostream>
#include <thread>
int main() {
  Server app;

  app.get("/", [](Request &req, Response &res) {
    std::cout << std::this_thread::get_id() << '\n';
    std::this_thread::sleep_for(std::chrono::seconds(3));
    res.send("Done");
  });

  app.listen(3000);
}
