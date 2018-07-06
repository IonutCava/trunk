#ifndef _SESSION_TPL_H_
#define _SESSION_TPL_H_

#include "WorldPacket.h"

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_wiarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio.hpp>
#include <deque>
#include <set>

using boost::asio::deadline_timer;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

namespace Divide {
//----------------------------------------------------------------------

class subscriber
{
public:
  virtual ~subscriber() {}
  virtual void sendPacket(const WorldPacket& p) = 0;
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

  void sendPacket(const WorldPacket& p)
  {
    std::for_each(std::begin(subscribers_), std::end(subscribers_),
        boost::bind(&subscriber::sendPacket, _1, boost::ref(p)));
  }

private:
  std::set<subscriber_ptr> subscribers_;
};

///This is a single session handled by the server. It is mapped to a single client
class tcp_session_tpl
  : public subscriber,
    public boost::enable_shared_from_this<tcp_session_tpl>
{
public:
  tcp_session_tpl(boost::asio::io_service& io_service, channel& ch);

  inline tcp::socket& getSocket() {  return socket_; }

  // Called by the server object to initiate the four actors.
  virtual void start();

  //Push a new packet in the output queue
  virtual void sendPacket(const WorldPacket& p);

  //Push a new file in the output queue
  virtual void sendFile(const stringImpl& fileName);

private:
  virtual void stop();
  virtual bool stopped() const;

  //Read Packet;
  virtual void start_read();
  virtual void handle_read_body(const boost::system::error_code& ec,size_t bytes_transfered);
  virtual void handle_read_packet(const boost::system::error_code& ec,size_t bytes_transfered);

  //Write Packet
  virtual void start_write();
  virtual void handle_write(const boost::system::error_code& ec);

  //Write File
  virtual void handle_write_file(const boost::system::error_code& ec);

  //Update Timers
  virtual void await_output();
  virtual void check_deadline(deadline_timer* deadline);

protected:

    //Define this functions to implement various packet handling (a switch statement for example)
    //switch(p.getOpcode()) { case SMSG_XXXXX: bla bla bla break; case MSG_HEARTBEAT: break;}
    virtual void handlePacket(WorldPacket& p) = 0;

private:
  size_t header;
  channel& channel_;
  tcp::socket socket_;
  boost::asio::streambuf input_buffer_;
  deadline_timer input_deadline_;
  std::deque<WorldPacket> output_queue_;
  std::deque<stringImpl> output_file_queue_;
  deadline_timer non_empty_output_queue_;
  deadline_timer output_deadline_;
  boost::asio::strand _strand;
  time_t start_time;
};

typedef boost::shared_ptr<tcp_session_tpl> tcp_session_ptr;

//----------------------------------------------------------------------

class udp_broadcaster  : public subscriber
{
public:
  udp_broadcaster(boost::asio::io_service& io_service,const udp::endpoint& broadcast_endpoint);

  inline udp::socket& getSocket() { return socket_; }
  void sendPacket(const WorldPacket& p);
private:

  udp::socket socket_;
};

}; //namespace Divide
#endif