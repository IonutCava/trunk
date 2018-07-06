#ifndef _SESSION_IMPL_H_
#define _SESSION_IMPL_H_

#include <DivideNetworking/tcp_session_tpl.h>

using boost::asio::deadline_timer;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

namespace Divide {

class tcp_session_impl : public tcp_session_tpl
{
public:
  tcp_session_impl(boost::asio::io_service& io_service, channel& ch);

private:

    void handlePacket(WorldPacket& p);

    void HandleHeartBeatOpCode(WorldPacket& p);
    void HandleDisconnectOpCode(WorldPacket& p);
    void HandleGeometryListOpCode(WorldPacket& p);
    void HandleRequestGeometry(WorldPacket& p);
    void HandlePingOpCode(WorldPacket& p);
};

}; //namespace Divide
#endif