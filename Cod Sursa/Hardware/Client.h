#ifndef _SESSION_H_
#define _SESSION_H_

#include "WorldPacket.h"
#include "OPCodes.h"
#include "Utility/Headers/ParamHandler.h"
#include <deque>

#include <boost/archive/text_iarchive.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp> 
#include <iostream>

using boost::asio::deadline_timer;
using boost::asio::ip::tcp;

class client
{
public:
  client(boost::asio::io_service& io_service, bool debugOutput)
    : stopped_(false),
	  _debugOutput(debugOutput),
      socket_(io_service),
      deadline_(io_service),
      heartbeat_timer_(io_service)
  {
  }

  // Start:: Called by the user of the client class to initiate the connection process.
  // The endpoint iterator will have been obtained using a tcp::resolver.
  // Stop:: This function terminates all the actors to shut down the connection. It
  // may be called by the user of the client class, or by the class itself in
  // response to graceful termination or an unrecoverable error.
  
  void start(tcp::resolver::iterator endpoint_iter);
  void stop();

  inline tcp::socket& getSocket(){  return socket_; }

  //Packet I/O
  void sendPacket(WorldPacket& p);
  void receivePacket(WorldPacket& p);
  void toggleDebugOutput(bool debugOutput){_debugOutput = debugOutput;}

private:

  //Connection
  void start_connect(tcp::resolver::iterator endpoint_iter);
  void handle_connect(const boost::system::error_code& ec, tcp::resolver::iterator endpoint_iter);

  //Read
  void start_read();
  void handle_read_body(const boost::system::error_code& ec,size_t bytes_transfered);
  void handle_read_packet(const boost::system::error_code& ec,size_t bytes_transfered);
  
  //Write
  void start_write();
  void handle_write(const boost::system::error_code& ec);

  //Timers
  void check_deadline();

private:
  bool stopped_,_debugOutput;
  tcp::socket socket_;
  size_t header;
  boost::asio::streambuf input_buffer_;
  deadline_timer deadline_;
  deadline_timer heartbeat_timer_;
  deque<WorldPacket> packetQueue;
};

#endif