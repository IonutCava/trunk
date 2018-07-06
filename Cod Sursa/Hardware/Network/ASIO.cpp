#include "ASIO.h"
#include "Utility/Headers/ParamHandler.h"
#include "OPCodes.h"

	void ASIO::init()
	{
		
		ParamHandler& par = ParamHandler::getInstance();
		try
		{
			tcp::resolver resolver(io_service);
			tcp::resolver::query query(par.getParam<string>("serverAdress"), "daytime");
			tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
			tcp::resolver::iterator end;

			socket = new tcp::socket(io_service);
			boost::system::error_code error = boost::asio::error::host_not_found;
			while (error && endpoint_iterator != end)
			{
				socket->close();
				socket->connect(*endpoint_iterator++, error);
			}
			if (error)
				throw boost::system::system_error(error);
			else
			{
				cout << "ASIO: Succesfully connected to server at address: " << par.getParam<string>("serverAdress") << endl;
				par.setParam("asioStatus", string("ASIO: Succesfully connected to server at address: " + par.getParam<string>("serverAdress"))); 
			}
		}
		catch (std::exception& e)
		{
			string t("ASIO Exception: "); t.append(e.what());
			cout << t << endl;
			par.setParam("asioStatus", t); 
		}
		WorldPacket p;
		p.Initialize(CMSG_PING);
		p << (U8)GETTIME();
		
	}

	string ASIO::getData()
	{
		ParamHandler& par = ParamHandler::getInstance();
		string t("");
		init();
		try
		{
			for (;;)
			{
				boost::array<char, 128> buf;
				boost::system::error_code error;

				size_t len = socket->read_some(boost::asio::buffer(buf), error);

				if (error == boost::asio::error::eof)
					break; // Connection closed cleanly by peer.
				else if (error)
					throw boost::system::system_error(error); // Some other error.
				else
					for(U32 i = 0; i < len; i++) t += buf[i];
			}
		}
		catch (std::exception& e)
		{
			string t("ASIO Exception: "); t.append(e.what());
			cout << t << endl;
			par.setParam("asioStatus", t); 
		}
		return t;
	}

