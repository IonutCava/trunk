#include "Headers/ASIOImpl.h"
#include <boost/archive/text_iarchive.hpp>
#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/MathHelper.h"
#include "Core/Headers/ApplicationTimer.h"
#include "Managers/Headers/SceneManager.h"

namespace Divide {

void ASIOImpl::handlePacket(WorldPacket& p) {
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
            ParamHandler::getInstance().setParam(
                _ID("serverResponse"),
                "Unknown OpCode: [ 0x" + std::to_string(p.opcode()) + " ]");
            break;
    };
}

void ASIOImpl::HandlePongOpCode(WorldPacket& p) {
    F32 time = 0;
    p >> time;
    D32 result = Time::ElapsedMilliseconds() - time;
    ParamHandler::getInstance().setParam(
        _ID("serverResponse"), "Server says: Pinged with : " +
                              std::to_string(floor(result + 0.5f)) +
                              " ms latency");
}

void ASIOImpl::HandleDisconnectOpCode(WorldPacket& p) {
    U8 code;
    p >> code;
    Console::printfn(Locale::get(_ID("ASIO_CLOSE")));
    if (code == 0) close();
    // else handleError(code);
}

void ASIOImpl::HandleGeometryAppendOpCode(WorldPacket& p) {
    Console::printfn(Locale::get(_ID("ASIO_PAK_REC_GEOM_APPEND")));
    U8 size;
    p >> size;
    vectorImpl<FileData> patch;
    for (U8 i = 0; i < size; i++) {
        FileData d;
        I8 type = -1;
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
        p >> type;
        if (type == 0)
            d.type = GeometryType::GEOMETRY;
        else if (type == 1)
            d.type = GeometryType::VEGETATION;
        else
            d.type = GeometryType::PRIMITIVE;
        p >> d.version;
        patch.push_back(d);
    }
    GET_ACTIVE_SCENE().addPatch(patch);
}

void ASIOImpl::HandleHeartBeatOpCode(WorldPacket& p) {
    /// nothing. Heartbeats keep us alive \:D/
}
};