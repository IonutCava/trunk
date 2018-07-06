#include "stdafx.h"

#include "Headers/Session.h"
#include "Headers/Server.h"
#include "Headers/OPCodesImpl.h"
#include "Headers/Patch.h"

#include "Networking/Headers/ASIO.h"

#include "Core/Resources/Headers/Resource.h"
#include <iostream>

namespace Divide {

Session::Session(boost::asio::io_service& io_service, channel& ch)
    : tcp_session_tpl(io_service, ch)
{
}

void Session::handlePacket(WorldPacket& p) {
    switch (p.opcode()) {
        case OPCodesEx::CMSG_GEOMETRY_LIST:
            HandleGeometryListOpCode(p);
            break;
        case OPCodesEx::CMSG_REQUEST_GEOMETRY:
            HandleRequestGeometry(p);
            break;
        default:
            tcp_session_tpl::handlePacket(p);
            break;
    };
}

void Session::HandleGeometryListOpCode(WorldPacket& p) {
    PatchData data;
    p >> data.sceneName;
    p >> data.size;
    ASIO::LOG_PRINT(("Received [ CMSG_GEOMERTY_LIST ] with : " + to_stringImpl(data.size) + " models").c_str());
    for (U32 i = 0; i < data.size; i++) {
        stringImpl name, modelname;
        p >> name;
        p >> modelname;
        data.name.push_back(name);
        data.modelName.push_back(modelname);
    }
    bool updated = Patch::instance().compareData(data);

    if (!updated) {
        WorldPacket r(OPCodesEx::SMSG_GEOMETRY_APPEND);

        vector<FileData> PatchData = Patch::instance().updateClient();
        r << PatchData.size();
        for (vector<FileData>::iterator _iter = std::begin(PatchData);
             _iter != std::end(PatchData); _iter++) {
            r << (*_iter).ItemName;
            r << (*_iter).ModelName;
            r << (*_iter).orientation.x;
            r << (*_iter).orientation.y;
            r << (*_iter).orientation.z;
            r << (*_iter).position.x;
            r << (*_iter).position.y;
            r << (*_iter).position.z;
            r << (*_iter).scale.x;
            r << (*_iter).scale.y;
            r << (*_iter).scale.z;
        }
        ASIO::LOG_PRINT(("Sending [SMSG_GEOMETRY_APPEND] with : " + to_stringImpl(PatchData.size()) + " models to update").c_str());
        sendPacket(r);
        Patch::instance().reset();
    }
}

void Session::HandleRequestGeometry(WorldPacket& p) {
    stringImpl file;
    p >> file;

    ASIO::LOG_PRINT(("Sending SMSG_SEND_FILE with item: " + file).c_str());
    WorldPacket r(OPCodesEx::SMSG_SEND_FILE);
    r << (U8)0;
    sendPacket(r);
    sendFile(file);
}

};  // namespace Divide
