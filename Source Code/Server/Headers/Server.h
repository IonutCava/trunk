#ifndef _SERVER_H_
#define _SERVER_H_

#include "Core/Headers/Singleton.h"

#include <DivideNetworking/tcp_session_tpl.h>
#include <boost/asio.hpp>

//----------------------------------------------------------------------
namespace Divide {

DEFINE_SINGLETON(Server)
  public:
    void init(U16 port, const stringImpl& broadcast_endpoint_address,
              bool debugOutput);

  private:
    Server();
    ~Server();
    void handle_accept(tcp_session_ptr session,
                       const boost::system::error_code& ec);

  private:
    boost::asio::io_service io_service_;
    tcp::acceptor* acceptor_;
    channel channel_;
    bool _debugOutput;

END_SINGLETON

};  // namespace Divide
#endif