#include "ASIO.h"
#include <boost/archive/text_iarchive.hpp>
#include "Utility/Headers/MathHelper.h"

	void ASIO::init()
	{
		ParamHandler& par = ParamHandler::getInstance();
		try
		{
			tcp::resolver r(io_service);
			c = new client(io_service);
			work.reset(new boost::asio::io_service::work(io_service));
		    c->start(r.resolve(tcp::resolver::query(par.getParam<string>("serverAdress"), "443")));
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
		string t = "ASIO: sent opcode [ 0x";
		t += code;
		t += "]";
		cout << t << endl;
		ParamHandler::getInstance().setParam("asioStatus",t);
	}

	void client::readPacket(WorldPacket& p)
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
			default:
				ParamHandler::getInstance().setParam("serverResponse",
					string("Unknown OpCode: [ 0x") + Util::toString(p.getOpcode()) + string(" ]"));
				break;
		};
	}

	void client::HandlePongOpCode(WorldPacket& p)
	{

		F32 time = 0;
		p >> time;
		F32 result = GETMSTIME() - time;
		ParamHandler::getInstance().setParam("serverResponse",string("Server says: Pinged with : ") + 
					Util::toString(floor(result+0.5f)) + string(" ms latency"));
	}

	void client::HandleDisconnectOpCode(WorldPacket& p)
	{
		U8 code;
		p >> code;
		cout << "CLOSING" << endl;
		if(code == 0) ASIO::getInstance().close();
		// else handleError(code);
	}

	void client::HandleHeartBeatOpCode(WorldPacket& p)
	{
		//nothing. Heartbeats keep us alive \:D/
	}