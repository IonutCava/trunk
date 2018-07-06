#ifndef _DIVIDE_BOOST_ASIO_H_
#define _DIVIDE_BOOST_ASIO_H_

#include "resource.h"
#include "Client.h"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "Utility/Headers/Singleton.h"
#include "WorldPacket.h"
#include "OPCodes.h"

using namespace boost;
using namespace boost::asio;
using namespace boost::asio::ip;

SINGLETON_BEGIN(ASIO)
	
public:
	void init(std::string& address, std::string& port);
	
	void sendPacket(WorldPacket& p);
	void connect(std::string& address, std::string& port){if(!_connected) init(address,port);}
	bool isConnected() {return _connected;}
	void disconnect();
	void toggleDebugOutput(bool debugOutput){_debugOutput = debugOutput; c->toggleDebugOutput(_debugOutput);}

private:
	//Singleton class: Constructor/Destructor private
	ASIO() {_connected = false; _debugOutput = true; c = NULL;}
	~ASIO() {work.reset(); t->join(); c->stop(); io_service.stop(); delete c; c = NULL;}

	friend class client;
	void close(){c->stop(); _connected = false;}


	void handlePacket(WorldPacket& p);

  void HandlePongOpCode(WorldPacket& p);
  void HandleHeartBeatOpCode(WorldPacket& p);
  void HandleDisconnectOpCode(WorldPacket& p);
  void HandleGeometryAppendOpCode(WorldPacket& p);

private:  	
	auto_ptr<io_service::work> work;
	boost::thread *t;
	bool _connected,_debugOutput;
	client *c;
	io_service io_service;
	std::string _address,_port;

SINGLETON_END()

#endif