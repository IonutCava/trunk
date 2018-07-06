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
  client(boost::asio::io_service& io_service)
    : stopped_(false),
      socket_(io_service),
      deadline_(io_service),
      heartbeat_timer_(io_service)
  {
  }

  // Called by the user of the client class to initiate the connection process.
  // The endpoint iterator will have been obtained using a tcp::resolver.
  void start(tcp::resolver::iterator endpoint_iter)
  {
    // Start the connect actor.
    start_connect(endpoint_iter);

    // Start the deadline actor. You will note that we're not setting any
    // particular deadline here. Instead, the connect and input actors will
    // update the deadline prior to each asynchronous operation.
    deadline_.async_wait(boost::bind(&client::check_deadline, this));
  }

  // This function terminates all the actors to shut down the connection. It
  // may be called by the user of the client class, or by the class itself in
  // response to graceful termination or an unrecoverable error.
  void stop()
  {
    stopped_ = true;
    socket_.close();
    deadline_.cancel();
    heartbeat_timer_.cancel();
  }

  tcp::socket& getSocket()
  {
	  return socket_;
  }
	void sendPacket(WorldPacket& p)
	{
		packetQueue.push_back(p);
		heartbeat_timer_.expires_at(boost::posix_time::neg_infin);
	}
	void readPacket(WorldPacket& p);

private:
  void start_connect(tcp::resolver::iterator endpoint_iter)
  {
    if (endpoint_iter != tcp::resolver::iterator())
    {
      std::cout << "Trying " << endpoint_iter->endpoint() << "...\n";

      // Set a deadline for the connect operation.
      deadline_.expires_from_now(boost::posix_time::seconds(60));

      // Start the asynchronous connect operation.
      socket_.async_connect(endpoint_iter->endpoint(),
          boost::bind(&client::handle_connect,
            this, _1, endpoint_iter));
    }
    else
    {
      // There are no more endpoints to try. Shut down the client.
      stop();
    }
  }

  void handle_connect(const boost::system::error_code& ec,
      tcp::resolver::iterator endpoint_iter)
  {
    if (stopped_)
      return;

    // The async_connect() function automatically opens the socket at the start
    // of the asynchronous operation. If the socket is closed at this time then
    // the timeout handler must have run first.
    if (!socket_.is_open())
    {
      std::cout << "Connect timed out\n";

      // Try the next available endpoint.
      start_connect(++endpoint_iter);
    }

    // Check if the connect operation failed before the deadline expired.
    else if (ec)
    {
      std::cout << "Connect error: " << ec.message() << "\n";

      // We need to close the socket used in the previous connection attempt
      // before starting a new one.
      socket_.close();

      // Try the next available endpoint.
      start_connect(++endpoint_iter);
    }

    // Otherwise we have successfully established a connection.
    else
    {
      std::cout << "Connected to " << endpoint_iter->endpoint() << "\n";

      // Start the input actor.
      start_read();

      // Start the heartbeat actor.
      start_write();
    }
  }

  void start_read()
  {
    // Set a deadline for the read operation.
    deadline_.expires_from_now(boost::posix_time::seconds(30));
	header = 0;
	input_buffer_.consume(input_buffer_.size());
    // Start an asynchronous operation to read a newline-delimited message.
    boost::asio::async_read(socket_,  boost::asio::buffer(&header,sizeof(header)),
        boost::bind(&client::handle_read_body, this, _1,
					boost::asio::placeholders::bytes_transferred));
  }

void handle_read_body(const boost::system::error_code& ec,size_t bytes_transfered)
  {
    if (stopped_)
      return;

    if (!ec)
    {
		deadline_.expires_from_now(boost::posix_time::seconds(30));
		boost::asio::async_read(socket_, input_buffer_.prepare( header ),
			boost::bind(&client::handle_read_packet,this, _1,
						boost::asio::placeholders::bytes_transferred));
    }
    else
    {
      stop();
    }
  }
 void handle_read_packet(const boost::system::error_code& ec,size_t bytes_transfered)
  {
    if (stopped_)
      return;

    if (!ec)
    {
		input_buffer_.commit(header);
		istream is(&input_buffer_);
		WorldPacket packet;
		try
		{
			boost::archive::text_iarchive ar(is);
			ar & packet;
			
		}
		catch(exception& e)
		{
			cout << e.what() << endl;
			cout << "ASIO: Received invalid packet!" << endl;
		}

		readPacket(packet);
	    start_read();
    }
    else
    {
      stop();
    }
}

  void start_write()
  {
    if (stopped_)
      return;

	if(packetQueue.empty())
	{
		WorldPacket heart(MSG_HEARTBEAT);
		heart << (I8)0;
		ParamHandler::getInstance().setParam("asioStatus",string("Sending HeartBeat"));
		packetQueue.push_back(heart);
	}

	WorldPacket& p = packetQueue.front();
	boost::asio::streambuf buf;
	std::ostream os( &buf );
	boost::archive::text_oarchive ar( os );
	ar & p; //Archive the packet

	size_t header = buf.size();
	std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back( boost::asio::buffer(&header, sizeof(header)) );
    buffers.push_back( buf.data() );
    boost::asio::async_write(socket_, buffers, boost::bind(&client::handle_write, this, _1));
  }

  void handle_write(const boost::system::error_code& ec)
  {
    if (stopped_)
      return;

    if (!ec)
    {
	  packetQueue.pop_front();
      heartbeat_timer_.expires_from_now(boost::posix_time::seconds(2));
      heartbeat_timer_.async_wait(boost::bind(&client::start_write, this));
    }
    else
    {
      std::cout << "Error on packet: " << ec.message() << "\n";
      stop();
    }
  }

  void check_deadline()
  {
    if (stopped_)
      return;

    // Check whether the deadline has passed. We compare the deadline against
    // the current time since a new asynchronous operation may have moved the
    // deadline before this actor had a chance to run.
    if (deadline_.expires_at() <= deadline_timer::traits_type::now())
    {
      // The deadline has passed. The socket is closed so that any outstanding
      // asynchronous operations are cancelled.
      socket_.close();

      // There is no longer an active deadline. The expiry is set to positive
      // infinity so that the actor takes no action until a new deadline is set.
      deadline_.expires_at(boost::posix_time::pos_infin);
    }

    // Put the actor back to sleep.
    deadline_.async_wait(boost::bind(&client::check_deadline, this));
  }

private:
	void HandlePongOpCode(WorldPacket& p);
	void HandleHeartBeatOpCode(WorldPacket& p);
	void HandleDisconnectOpCode(WorldPacket& p);

private:
  bool stopped_;
  tcp::socket socket_;
  size_t header;
  boost::asio::streambuf input_buffer_;
  deadline_timer deadline_;
  deadline_timer heartbeat_timer_;
  std::deque<WorldPacket> packetQueue;
};

#endif