#include "ASIO.h"
#include <boost/archive/text_iarchive.hpp>
#include "Utility/Headers/MathHelper.h"
#include "Utility/Headers/BaseClasses.h"
#include "Managers/SceneManager.h"

	void ASIO::init(std::string& address, std::string& port)
	{
		ParamHandler& par = ParamHandler::getInstance();

		try
		{
			tcp::resolver r(io_service);
			c = new client(io_service,_debugOutput);
			work.reset(new boost::asio::io_service::work(io_service));
			c->start(r.resolve(tcp::resolver::query(address, port.c_str())));
			t = new boost::thread(boost::bind(&boost::asio::io_service::run, &io_service));
		    io_service.poll();
			_connected = true;
		}
		catch (std::exception& e)
		{
			string t("ASIO Exception: ");
			t.append(e.what());
			cout << t << endl;
			par.setParam("asioStatus", t); 
		}
	}

	void ASIO::disconnect()
	{
		if(!_connected) return;
		WorldPacket p(CMSG_REQUEST_DISCONNECT);
		p << c->getSocket().local_endpoint().address().to_string();
		sendPacket(p);
	}

	void ASIO::sendPacket(WorldPacket& p)
	{
		if(!_connected) return;
		c->sendPacket(p);
		char code[3];
		_itoa(p.getOpcode(),code,3);
		string t = "ASIO: sent opcode [ 0x" + Util::toString(code) + string("]");
		cout << t << endl;
		ParamHandler::getInstance().setParam("asioStatus",t);
	}

	void ASIO::handlePacket(WorldPacket& p)
	{
		switch(p.getOpcode())
		{
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
					string("Unknown OpCode: [ 0x") + Util::toString(p.getOpcode()) + string(" ]"));
				break;
		};
	}

	void ASIO::HandlePongOpCode(WorldPacket& p)
	{

		F32 time = 0;
		p >> time;
		F32 result = GETMSTIME() - time;
		ParamHandler::getInstance().setParam("serverResponse",string("Server says: Pinged with : ") + 
					Util::toString(floor(result+0.5f)) + string(" ms latency"));
	}

	void ASIO::HandleDisconnectOpCode(WorldPacket& p)
	{
		U8 code;
		p >> code;
		cout << "CLOSING" << endl;
		if(code == 0) ASIO::getInstance().close();
		// else handleError(code);
	}

	void ASIO::HandleGeometryAppendOpCode(WorldPacket& p)
	{
		cout << "ASIO: received  [SMSG_GEOMETRY_APPEND]" << endl;
		U32 size;
		p >> size;
		vector<FileData> patch;
		for(U32 i = 0; i < size; i++)
		{
			FileData d;
			int type = -1;
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
			p >> type; if(type = 0) d.type = MESH; else d.type = VEGETATION;
			p >> d.version;
			patch.push_back(d);
		}
		SceneManager::getInstance().addPatch(patch);
	}

	void ASIO::HandleHeartBeatOpCode(WorldPacket& p)
	{
		//nothing. Heartbeats keep us alive \:D/
	}