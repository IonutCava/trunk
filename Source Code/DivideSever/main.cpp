#include "Server.h"
#include "Utility/Headers/OutputBuffer.h"

using namespace Divide;

int main()
{
    /*Console output redirect*/
    std::ofstream myfile("console_server.log");
    outbuf obuf(myfile.rdbuf(), std::cout.rdbuf());
    scoped_streambuf_assignment ssa(std::cout, &obuf);
    /*Console output redirect*/
    std::string address = "127.0.0.1";

    Server::getInstance().init(((U16)443),address,true);
    return 0;
}