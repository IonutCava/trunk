#ifndef _SESSION_H_
#define _SESSION_H_

#include "Utility/resource.h"
#include "WorldPacket.h"
#include "OPCodes.h"

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_wiarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio.hpp>

using boost::asio::deadline_timer;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

//----------------------------------------------------------------------

class subscriber
{
public:
  virtual ~subscriber() {}
  virtual void sendPacket(WorldPacket& p) = 0;
};

typedef boost::shared_ptr<subscriber> subscriber_ptr;

//----------------------------------------------------------------------

class channel
{
public:
  void join(subscriber_ptr subscriber)
  {
    subscribers_.insert(subscriber);
  }

  void leave(subscriber_ptr subscriber)
  {
    subscribers_.erase(subscriber);
  }

  void sendPacket(WorldPacket& p)
  {
    std::for_each(subscribers_.begin(), subscribers_.end(),
        boost::bind(&subscriber::sendPacket, _1, boost::ref(p)));
  }

private:
  std::set<subscriber_ptr> subscribers_;
};

class tcp_session
  : public subscriber,
    public boost::enable_shared_from_this<tcp_session>
{
public:
  tcp_session(boost::asio::io_service& io_service, channel& ch);

  inline tcp::socket& getSocket() {  return socket_; }

  // Called by the server object to initiate the four actors.
  void start();

  //Push a new packet in the output queue
  void sendPacket(WorldPacket& p);

private:
  void stop();
  bool stopped() const;

  //Read Packet;
  void start_read();
  void handle_read_body(const boost::system::error_code& ec,size_t bytes_transfered);
  void handle_read_packet(const boost::system::error_code& ec,size_t bytes_transfered);

  //Write Packet
  void start_write();
  void handle_write(const boost::system::error_code& ec);

  //Update Timers
  void await_output();
  void check_deadline(deadline_timer* deadline);

private:
	
	//Define this functions to implement various packet handling (a switch statement for example)
	//switch(p.getOpcode()) { case SMSG_XXXXX: bla bla bla break; case MSG_HEARTBEAT: break;}
	void handlePacket(WorldPacket& p);

private:
  size_t header;
  channel& channel_;
  tcp::socket socket_;
  boost::asio::streambuf input_buffer_;
  deadline_timer input_deadline_;
  std::deque<WorldPacket> output_queue_;
  deadline_timer non_empty_output_queue_;
  deadline_timer output_deadline_;
  boost::asio::strand _strand;
  time_t start_time;
};

typedef boost::shared_ptr<tcp_session> tcp_session_ptr;

//----------------------------------------------------------------------

class udp_broadcaster  : public subscriber
{
public:
  udp_broadcaster(boost::asio::io_service& io_service,const udp::endpoint& broadcast_endpoint);

  inline udp::socket& getSocket() { return socket_; }
  void sendPacket(WorldPacket& p);
private:

  udp::socket socket_;
};


#endif