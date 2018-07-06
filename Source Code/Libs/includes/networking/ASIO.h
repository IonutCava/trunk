#ifndef _DIVIDE_BOOST_ASIO_H_
#define _DIVIDE_BOOST_ASIO_H_

#include "core.h"
#include "WorldPacket.h"
#include "OPCodes.h"
#include "Client.h"
#include <boost/asio.hpp>
#include <boost/thread.hpp>


using namespace boost;
using namespace boost::asio;
using namespace boost::asio::ip;

DEFINE_SINGLETON(ASIO)
	
public:
	void init(std::string& address, std::string& port);
	
	void sendPacket(WorldPacket& p);
	void connect(std::string& address, std::string& port){if(!_connected) init(address,port);}
	bool isConnected() {return _connected;}
	void disconnect();
	void toggleDebugOutput(bool debugOutput){_debugOutput = debugOutput; c->toggleDebugOutput(_debugOutput);}

private:
	///Singleton class: Constructor/Destructor private
	ASIO() {_connected = false; _debugOutput = true; c = NULL;}
	~ASIO() {work.reset(); t->join(); c->stop(); io_service_.stop(); delete c; c = NULL;}

	friend class client;
	void close(){c->stop(); _connected = false;}

	///Define this functions to implement various packet handling (a switch statement for example)
	///switch(p.getOpcode()) { case SMSG_XXXXX: bla bla bla break; case MSG_HEARTBEAT: break;}
	void handlePacket(WorldPacket& p);

	void HandlePongOpCode(WorldPacket& p);
	void HandleHeartBeatOpCode(WorldPacket& p);
	void HandleDisconnectOpCode(WorldPacket& p);
	void HandleGeometryAppendOpCode(WorldPacket& p);

private:  	
	std::auto_ptr<io_service::work> work;
	boost::thread *t;
	bool _connected,_debugOutput;
	client *c;
	io_service io_service_;
	std::string _address,_port;

END_SINGLETON

#endif
