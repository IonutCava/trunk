#include "config.h"

#include "Headers/Server.h"

#include "Utility/Headers/OutputBuffer.h"

#include <fstream>

int main() {
    //Console output redirect
    std::ofstream stdConsole(SERVER_LOG_FILE,  std::ofstream::out | std::ofstream::trunc);
    std::cout.rdbuf(stdConsole.rdbuf());

    stringImpl address("127.0.0.1");
    Divide::Server::instance().init(((Divide::U16)443), address, true);
    return 0;
}
