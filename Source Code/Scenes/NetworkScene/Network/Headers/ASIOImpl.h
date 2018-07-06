#ifndef _DIVIDE_BOOST_ASIO_IMPL_H_
#define _DIVIDE_BOOST_ASIO_IMPL_H_

#include "OPCodesImpl.h"
#include "Networking/Headers/ASIO.h"

namespace Divide {

DEFINE_SINGLETON(ASIOImpl, ASIO)
  private:
    /// Singleton class: Constructor/Destructor private
    ASIOImpl() : ASIO() {}

    ~ASIOImpl() {}

    /// Define this functions to implement various packet handling (a switch
    /// statement for example)
    /// switch(p.getOpcode()) { case SMSG_XXXXX: bla bla bla break; case
    /// MSG_HEARTBEAT: break;}
    void handlePacket(WorldPacket& p);

    void HandlePongOpCode(WorldPacket& p);
    void HandleHeartBeatOpCode(WorldPacket& p);
    void HandleDisconnectOpCode(WorldPacket& p);
    void HandleGeometryAppendOpCode(WorldPacket& p);

END_SINGLETON

};  // namespace Divide
#endif
