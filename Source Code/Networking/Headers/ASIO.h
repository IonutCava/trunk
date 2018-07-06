#ifndef _DIVIDE_BOOST_ASIO_TPL_H_
#define _DIVIDE_BOOST_ASIO_TPL_H_

#include "WorldPacket.h"

#include <boost/asio.hpp>
#include <thread>

namespace Divide {

#ifndef OPCODE_ENUM
#error \
    "Please include 'OPCodesTpl' and define custom OPcodes before using the networking library!"
#endif

class OPCodes;
class Client;
class ASIO {
   public:
    /// Send a packet to the target server
    virtual bool sendPacket(WorldPacket& p) const;
    /// Init a connection to the target address:port
    virtual bool init(const stringImpl& address, U16 port);
    /// Connect to target address:port only if we have a new IP:PORT combo or
    /// our connection timed out
    virtual bool connect(const stringImpl& address, U16 port);
    /// Disconnect from the server
    virtual void disconnect();
    /// Check connection state;
    virtual bool isConnected() const;
    /// Toggle the printing of debug information
    virtual void toggleDebugOutput(const bool debugOutput);

   protected:
    ASIO();
    virtual ~ASIO();

    friend class Client;
    void close();

    // Define this functions to implement various packet handling (a switch
    // statement for example)
    // switch(p.getOpcode()) { case SMSG_XXXXX: bla bla bla break; case
    // MSG_HEARTBEAT: break;}
    virtual void handlePacket(WorldPacket& p) = 0;

   protected:
    Client* _localClient;
    std::auto_ptr<boost::asio::io_service::work> _work;
    std::thread* _thread;
    bool _connected;
    bool _debugOutput;
    boost::asio::io_service io_service_;
    stringImpl _address, _port;
};

};  // namespace Divide
#endif
