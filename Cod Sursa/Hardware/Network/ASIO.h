#ifndef _DIVIDE_BOOST_ASIO_H_
#define _DIVIDE_BOOST_ASIO_H_

#include "resource.h"
#include "Session.h"
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include "Utility/Headers/Singleton.h"
#include "WorldPacket.h"
#include "OPCodes.h"
#include "Utility/Headers/ParamHandler.h"
#include <boost/thread.hpp>

using namespace boost;
using namespace boost::asio;
using namespace boost::asio::ip;

SINGLETON_BEGIN(ASIO)
	
public:
	ASIO() {_connected = false; c = NULL; init();}
	~ASIO() {work.reset(); t->join(); c->stop(); io_service.stop(); delete c; c = NULL;}
	void sendPacket(WorldPacket& p);
	
private:
	void init();
 private:  	
	auto_ptr<boost::asio::io_service::work> work;
	boost::thread *t;
	bool _connected;
	client *c;
	io_service io_service;

SINGLETON_END()

#endif