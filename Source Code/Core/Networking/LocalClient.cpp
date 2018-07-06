#include "stdafx.h"

#include "Headers/LocalClient.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/Resource.h"
#include "Core/Time/Headers/ApplicationTimer.h"

namespace Divide {

LocalClient::LocalClient(Kernel& parent) : ASIO(), KernelComponent(parent)
{
}

LocalClient::~LocalClient()
{
}

void LocalClient::handlePacket(WorldPacket& p) {
    switch (p.opcode()) {
    case OPCodes::MSG_HEARTBEAT:
        HandleHeartBeatOpCode(p);
        break;
    case OPCodesEx::SMSG_PONG:
        HandlePongOpCode(p);
        break;
    case OPCodes::SMSG_DISCONNECT:
        HandleDisconnectOpCode(p);
        break;
    case OPCodesEx::SMSG_GEOMETRY_APPEND:
        HandleGeometryAppendOpCode(p);
        break;
    default:
        ParamHandler::instance().setParam(_ID("serverResponse"),
                                          "Unknown OpCode: [ 0x" + to_stringImpl(p.opcode()) + " ]");
        break;
    };
}

void LocalClient::HandlePongOpCode(WorldPacket& p) {
    F32 time = 0;
    p >> time;
    D64 result = Time::ElapsedMilliseconds() - time;
    ParamHandler::instance().setParam(
        _ID("serverResponse"), "Server says: Pinged with : " +
        to_stringImpl(floor(result + 0.5f)) +
        " ms latency");
}

void LocalClient::HandleDisconnectOpCode(WorldPacket& p) {
    U8 code;
    p >> code;
    Console::printfn(Locale::get(_ID("ASIO_CLOSE")));
    if (code == 0) close();
    // else handleError(code);
}

void LocalClient::HandleGeometryAppendOpCode(WorldPacket& p) {
    Console::printfn(Locale::get(_ID("ASIO_PAK_REC_GEOM_APPEND")));
    U8 size;
    p >> size;
    vector<FileData> patch;
    for (U8 i = 0; i < size; i++) {
        FileData d;
        p >> d.ItemName;
        p >> d.ModelName;
        p >> d.orientation.x;
        p >> d.orientation.y;
        p >> d.orientation.z;
        p >> d.position.x;
        p >> d.position.y;
        p >> d.position.z;
        p >> d.scale.x;
        p >> d.scale.y;
        p >> d.scale.z;
        patch.push_back(d);
    }
    //_parentScene.addPatch(patch);
}

void LocalClient::HandleHeartBeatOpCode(WorldPacket& p) {
    /// nothing. Heartbeats keep us alive \:D/
}

}; //namespace Divide