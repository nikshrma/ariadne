#include "request.h"
#include "response.h"
#include "server.h"
int main() {
  Server server;

  server.get("/", [](Request &req, Response &res) {
    res.status(200)
        .set("Content-Type", "text/plain")
        .set("X-Powered-By", "Ariadne")
        .send("Ariadne says hi!");
  });

  server.listen(3000);
}
