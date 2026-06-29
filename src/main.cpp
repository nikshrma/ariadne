#include "request.h"
#include "response.h"
#include "server.h"
int main() {
  Server server;
  server.get("/",
             [](Request &req, Response &res) { res.send("this is home"); });
  server.post("/", [](Request &req, Response &res) { res.send("Post home"); });
  server.remove("/",
                [](Request &req, Response &res) { res.send("Delete home"); });
  server.put("/", [](Request &req, Response &res) { res.send("Put home"); });
  server.listen(3000);
}
