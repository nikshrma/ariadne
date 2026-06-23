#include "server.h"
#include <iostream>
int main() {
  Server server;
  server.listen(3000);
  std::string s;
  std::getline(std::cin, s);
}
