#ifndef OPCODE_ENUM
#define OPCODE_ENUM OPcodes
#endif
// There doesn't seem to be any other way to turn this off such that the presence of
// the user-defined operator,() below doesn't cause spurious warning all over the place,
// so unconditionally turn it off.
#if defined(_MSC_VER)
#	pragma warning( push )
// user defined binary operator ',' exists but no overload could convert all operands, default built-in binary operator ',' used
#		pragma warning(disable:4913) 
#elif defined(__GNUC__)
#	pragma GCC diagnostic push
#		//pragma GCC diagnostic ignored "-Wall"
#endif

#include "OPCodesTpl.h"
#include "ASIO.h"
#include "Client.h"
#include <boost/archive/text_iarchive.hpp>

ASIO::ASIO() : _connected(false), _debugOutput(true), _localClient(NULL)
{
}

ASIO::~ASIO()
{
	_work.reset();
	_thread->join();
	_localClient->stop();
	io_service_.stop();
	delete _localClient;
	_localClient = NULL;
}

void ASIO::disconnect(){
	if(!_connected) return;
	WorldPacket p(CMSG_REQUEST_DISCONNECT);
	p << _localClient->getSocket().local_endpoint().address().to_string();
	sendPacket(p);
}

void ASIO::init(const std::string& address,const std::string& port){
	try{
		tcp::resolver res(io_service_);
		_localClient = new Client(this, io_service_,_debugOutput);
		_work.reset(new boost::asio::io_service::work(io_service_));
	    _localClient->start(res.resolve(tcp::resolver::query(address, port)));
		_thread = new boost::thread(boost::bind(&boost::asio::io_service::run, &io_service_));
	    io_service_.poll();
		_connected = true;
	}catch (std::exception& e){
		if(_debugOutput){
			std::string msg("ASIO Exception: ");
			msg.append(e.what());
			std::cout << msg << std::endl;
		}
	}
}

void ASIO::connect(const std::string& address,const std::string& port){
	if(!_connected) {
		init(address,port);
	}
}

bool ASIO::isConnected() const {
	return _connected;
}

void ASIO::close(){
	_localClient->stop();
	_connected = false;
}

void ASIO::sendPacket(WorldPacket& p)const {
	if(!_connected) return;

	_localClient->sendPacket(p);

	if(_debugOutput){
		std::string msg = "ASIO: sent opcode [ 0x" + DivideNetworking::toString(p.getOpcode()) + std::string("]");
		std::cout << msg << std::endl;
	}
}

void ASIO::toggleDebugOutput(const bool debugOutput){
	_debugOutput = debugOutput;
	_localClient->toggleDebugOutput(_debugOutput);
}

#if defined(_MSC_VER)
#	pragma warning( pop )
#elif defined(__GNUC__)
#	pragma GCC diagnostic pop
#endif