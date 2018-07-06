#include "Server.h"
#include <Headers/OPCodesImpl.h>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include "Utility/Headers/Patch.h"

tcp_session_impl::tcp_session_impl(boost::asio::io_service& io_service, channel& ch) : tcp_session_tpl(io_service,ch)
{
}

void tcp_session_impl::handlePacket(WorldPacket& p)
{
	switch(p.getOpcode())
	{
		case MSG_HEARTBEAT:
			cout << "Received [ MSG_HEARTBEAT ]" << endl;
			HandleHeartBeatOpCode(p);
			break;
		case CMSG_PING:
			cout << "Received [ CMSG_PING ]" << endl;
			HandlePingOpCode(p);
			break;
		case CMSG_REQUEST_DISCONNECT:
			HandleDisconnectOpCode(p);
			break;
		case CMSG_GEOMETRY_LIST:
			HandleGeometryListOpCode(p);
			break;
		case CMSG_REQUEST_GEOMETRY:
			HandleRequestGeometry(p);
			break;
		default:
			cout << "Received unknow OPCode [ 0x" << p.getOpcode() << " ]" << endl;
			break;
	};
}
void tcp_session_impl::HandleHeartBeatOpCode(WorldPacket& p)
{
	WorldPacket r(MSG_HEARTBEAT);
	cout << "Sending  [ MSG_HEARTBEAT]" << endl;
	r << (I8)0;
	sendPacket(r);
}

void tcp_session_impl::HandlePingOpCode(WorldPacket& p)
{
	F32 time = 0;
	p >> time;
	cout << "Sending  [ SMSG_PONG ] with data: " << time << endl;
	WorldPacket r(SMSG_PONG);
	r << time;
	sendPacket(r);
}

void tcp_session_impl::HandleDisconnectOpCode(WorldPacket& p)
{
	string client;
	p >> client;
	cout << "Received [ CMSG_REQUEST_DISCONNECT ] from: [ " << client << " ]" << endl;
	WorldPacket r(SMSG_DISCONNECT);
	r << (U8)0; //this will be the error code returned after safely saving client
	sendPacket(r);
}

void tcp_session_impl::HandleGeometryListOpCode(WorldPacket& p)
{
	PatchData data;
	p >> data.sceneName;
	p >> data.size;
	cout << "Received [ CMSG_GEOMERTY_LIST ] with : " << data.size << " models" << endl;
	for(U32 i = 0; i < data.size; i++)
	{
		string name, modelname;
		U32 version = 0;
		p >> name;
		p >> modelname;
		p >> version;
		data.name.push_back(name);
		data.modelName.push_back(modelname);
		data.version.push_back(version);
	}
	bool updated = Patch::getInstance().compareData(data);

	if(!updated)
	{
		WorldPacket r(SMSG_GEOMETRY_APPEND);

		vector<FileData> PatchData = Patch::getInstance().updateClient();
		r << PatchData.size();
		for(vector<FileData>::iterator _iter = PatchData.begin(); _iter != PatchData.end(); _iter++)
		{
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
			if((*_iter).type == MESH)
				r << 0;
			else if((*_iter).type == VEGETATION)
				r << 1;
			else
				r << 2;
			r << (*_iter).version;
		}
		cout << "Sending [SMSG_GEOMETRY_APPEND] with : " << PatchData.size() << " models to update" << endl;
		sendPacket(r);
		Patch::getInstance().reset();
	}
}

void tcp_session_impl::HandleRequestGeometry(WorldPacket& p)
{
	std::string file;
	p >> file;

	cout << "Sending SMSG_SEND_FILE with item: " << file << endl;
	WorldPacket r(SMSG_SEND_FILE);
	r << (U8)0;
	sendPacket(r);
	sendFile(file);
}