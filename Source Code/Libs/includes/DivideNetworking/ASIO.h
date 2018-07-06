#ifndef _DIVIDE_BOOST_ASIO_TPL_H_
#define _DIVIDE_BOOST_ASIO_TPL_H_

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "WorldPacket.h"

using namespace boost::asio;
using namespace boost::asio::ip;

namespace Divide {

#ifndef OPCODE_ENUM
#error "Please include 'OPCodesTpl' and define custom OPcodes before using the networking library!"
#endif

enum  OPCodes;
class Client;
class ASIO {
public:
	///Send a packet to the target server
	virtual void sendPacket(WorldPacket& p) const;
	///Init a connection to the target address:port
	virtual void init(const std::string& address,const std::string& port);
	///Connect to target address:port only if we have a new IP:PORT combo or our connection timed out
	virtual void connect(const std::string& address,const std::string& port);
	///Disconnect from the server
	virtual void disconnect();
	///Check connection state;
	virtual bool isConnected() const;
	///Toggle the printing of debug information
	virtual void toggleDebugOutput(const bool debugOutput);

protected:
	ASIO();
	virtual ~ASIO();

	friend class Client;
	void close();

	//Define this functions to implement various packet handling (a switch statement for example)
	//switch(p.getOpcode()) { case SMSG_XXXXX: bla bla bla break; case MSG_HEARTBEAT: break;}
	virtual void handlePacket(WorldPacket& p) = 0;

protected:
	Client *_localClient;
	std::auto_ptr<io_service::work> _work;
	boost::thread *_thread;
	bool _connected;
	bool _debugOutput;
	io_service io_service_;
	std::string _address,_port;
};

}; //namespace Divide
#endif
