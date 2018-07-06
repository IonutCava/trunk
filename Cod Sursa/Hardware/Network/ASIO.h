#ifndef _DIVIDE_BOOST_ASIO_H_
#define _DIVIDE_BOOST_ASIO_H_

#include "resource.h"
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include "Utility/Headers/Singleton.h"
#include "WorldPacket.h"

using boost::asio::ip::tcp;
using boost::asio::ip::udp;

SINGLETON_BEGIN(ASIO)
	
public:
	~ASIO() {socket->close(); delete socket; socket = NULL; socket_backup->close(); delete socket_backup;}
	void init();
	string getData();

private:
	tcp::socket* socket;
	udp::socket* socket_backup;
	boost::asio::io_service io_service;
SINGLETON_END()

#endif