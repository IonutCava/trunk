#include <networking/ASIO.h>
#include <boost/archive/text_iarchive.hpp>
#include "Core/Math/Headers/MathHelper.h"
#include "Managers/Headers/SceneManager.h"

void ASIO::disconnect(){

	if(!_connected) return;
	WorldPacket p(CMSG_REQUEST_DISCONNECT);
	p << c->getSocket().local_endpoint().address().to_string();
	sendPacket(p);
}

void ASIO::handlePacket(WorldPacket& p){

	switch(p.getOpcode()){
		case MSG_HEARTBEAT:
			HandleHeartBeatOpCode(p);
			break;
		case SMSG_PONG:
			HandlePongOpCode(p);
			break;
		case SMSG_DISCONNECT:
			HandleDisconnectOpCode(p);
			break;
		case SMSG_GEOMETRY_APPEND:
			HandleGeometryAppendOpCode(p);
			break;
		default:
			ParamHandler::getInstance().setParam("serverResponse",
				std::string("Unknown OpCode: [ 0x") + Util::toString(p.getOpcode()) + std::string(" ]"));
			break;
	};
}

void ASIO::HandlePongOpCode(WorldPacket& p){

	F32 time = 0;
	p >> time;
	F32 result = GETMSTIME() - time;
	ParamHandler::getInstance().setParam("serverResponse",std::string("Server says: Pinged with : ") + 
					Util::toString(floor(result+0.5f)) + std::string(" ms latency"));
}

void ASIO::HandleDisconnectOpCode(WorldPacket& p){
	U8 code;
	p >> code;
	PRINT_FN(Locale::get("ASIO_CLOSE"));
	if(code == 0) ASIO::getInstance().close();
	// else handleError(code);
}

void ASIO::HandleGeometryAppendOpCode(WorldPacket& p){
	PRINT_FN(Locale::get("ASIO_PAK_REC_GEOM_APPEND"));
	U8 size;
	p >> size;
	std::vector<FileData> patch;
	for(U8 i = 0; i < size; i++){

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
		if(type == 0) d.type = GEOMETRY;
		else if(type == 1) d.type = VEGETATION;
		else d.type = PRIMITIVE;
		p >> d.version;
		patch.push_back(d);
	}
	GET_ACTIVE_SCENE()->addPatch(patch);
}

void ASIO::HandleHeartBeatOpCode(WorldPacket& p){
	///nothing. Heartbeats keep us alive \:D/
}