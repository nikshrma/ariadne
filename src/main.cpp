#include "request.h"
#include "response.h"
#include "server.h"
int main() {
  Server server;
  server.get("/",
             [](Request &req, Response &res) { res.send("this is home"); });
  server.listen(3000);
}
