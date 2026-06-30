#include "request.h"
#include "response.h"
#include "server.h"
int main() {
  Server server;

  server.get("/users/:id",
             [](Request &req, Response &res) { res.send(req.params["id"]); });

  server.listen(3000);
}
