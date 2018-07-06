#include "ASIO.h"
#include <boost/archive/text_iarchive.hpp>

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
		bool changed = false;
		string response = "Server says: ";
		U32 time = 0;
		char code[4];
		switch(p.getOpcode())
		{
			case MSG_HEARTBEAT:
				//nothing. Heartbeats keep us alive \:D/
				break;
			case SMSG_PONG:
				p >> time;
				_itoa(time,code,4);
				response += "Pinged at : " ; 
				response += code; 
				response += " server time";
				changed = true;
				break;
			default:
				response += "Unknown OpCode: [ 0x";
				response += _itoa(p.getOpcode(),code,3);
				response += " ]";
				changed = true;
				break;
		};
		if(changed)ParamHandler::getInstance().setParam("serverResponse",response);
		free(code);
		response.clear();
	}

