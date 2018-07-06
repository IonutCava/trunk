#ifndef _SERVER_H_
#define _SERVER_H_
#include "Utility/resource.h"
#include "Utility/Singleton.h"

#include "tcp_session.h"
#include "OPCodes.h"
#include <boost/asio.hpp>
//----------------------------------------------------------------------

SINGLETON_BEGIN(SERVER)


public:
	void init(U16 port, std::string& broadcast_endpoint_address);

private:
  SERVER();
  ~SERVER();
  void handle_accept(tcp_session_ptr session, const boost::system::error_code& ec);

private:
  boost::asio::io_service io_service_;
  tcp::acceptor* acceptor_;
  channel channel_;

SINGLETON_END()

#endif