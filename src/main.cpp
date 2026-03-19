#include "../include/Server.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

bool parsePort(const std::string &portStr, int &outPort) {
  const int portMin = 1024;
  const int portMax = 65535;
  std::istringstream iss(portStr);

  iss >> outPort;

  if (iss.fail() || !iss.eof())
    return false;

  return (outPort >= portMin && outPort <= portMax);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Error: Invalid number of arguments." << std::endl;
    std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
    return EXIT_FAILURE;
  }

  std::string portStr(argv[1]);
  std::string password(argv[2]);

  int port;
  if (parsePort(portStr, port) == false) {
    std::cerr << "Error: Invalid port -> " << portStr << std::endl;
    std::cerr << "You have to pass valid port number. (min:1024, max:65535)";
    return EXIT_FAILURE;
  }

  if (password.empty()) {
    std::cerr << "Error: password cannot be empty." << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Server is ready to start." << std::endl;

  try {
    Server ircServer(port, password);
    ircServer.start();
  } catch(const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
