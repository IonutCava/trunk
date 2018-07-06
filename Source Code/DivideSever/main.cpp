#include "Server.h"
#include "Utility/Headers/OutputBuffer.h"

int main()
{
    /*Console output redirect*/
    ofstream myfile("console_server.log");
    outbuf obuf(myfile.rdbuf(), cout.rdbuf());
    scoped_streambuf_assignment ssa(cout, &obuf);
    /*Console output redirect*/
    std::string address = "127.0.0.1";

    Server::getInstance().init(((U16)443),address,true);
    return 0;
}