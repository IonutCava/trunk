#pragma once
#ifndef _SESSION_TPL_H_
#define _SESSION_TPL_H_

#include "WorldPacket.h"

namespace Divide {
//----------------------------------------------------------------------

//This is game specific but core functionality
extern void UpdateEntities(WorldPacket& p);

class subscriber {
   public:
    virtual ~subscriber() {}
    virtual void sendPacket(const WorldPacket& p) = 0;
};

typedef std::shared_ptr<subscriber> subscriber_ptr;

//----------------------------------------------------------------------

class channel {
   public:
    void join(subscriber_ptr sub) { subscribers_.insert(sub); }

    void leave(subscriber_ptr sub) { subscribers_.erase(sub); }

    void sendPacket(const WorldPacket& p) {
        std::for_each(std::begin(subscribers_), std::end(subscribers_),
                      boost::bind(&subscriber::sendPacket, _1, boost::ref(p)));
    }

   private:
    std::set<subscriber_ptr> subscribers_;
};

/// This is a single session handled by the server. It is mapped to a single
/// client
class tcp_session_tpl : public subscriber,
                        public std::enable_shared_from_this<tcp_session_tpl> {
   public:
    tcp_session_tpl(boost::asio::io_service& io_service, channel& ch);

    inline boost::asio::ip::tcp::socket& getSocket() { return _socket; }

    // Called by the server object to initiate the four actors.
    virtual void start();

    // Push a new packet in the output queue
    virtual void sendPacket(const WorldPacket& p);

    // Push a new file in the output queue
    virtual void sendFile(const stringImpl& fileName);

   private:
    virtual void stop();
    virtual bool stopped() const;

    // Read Packet;
    virtual void start_read();
    virtual void handle_read_body(const boost::system::error_code& ec,
                                  size_t bytes_transfered);
    virtual void handle_read_packet(const boost::system::error_code& ec,
                                    size_t bytes_transfered);

    // Write Packet
    virtual void start_write();
    virtual void handle_write(const boost::system::error_code& ec);

    // Write File
    virtual void handle_write_file(const boost::system::error_code& ec);

    // Update Timers
    virtual void await_output();
    virtual void check_deadline(boost::asio::deadline_timer* deadline);

   protected:
    // Define this functions to implement various packet handling (a switch
    // statement for example)
    // switch(p.getOpcode()) { case SMSG_XXXXX: bla bla bla break; case
    // MSG_HEARTBEAT: break;}
    virtual void handlePacket(WorldPacket& p);

    virtual void HandleHeartBeatOpCode(WorldPacket& p);
    virtual void HandleDisconnectOpCode(WorldPacket& p);
    virtual void HandlePingOpCode(WorldPacket& p);
    virtual void HandleEntityUpdateOpCode(WorldPacket& p);

   private:
    size_t _header;
    channel& _channel;
    boost::asio::ip::tcp::socket _socket;
    boost::asio::streambuf _inputBuffer;
    boost::asio::deadline_timer _inputDeadline;
    std::deque<WorldPacket> _outputQueue;
    std::deque<stringImpl> _outputFileQueue;
    boost::asio::deadline_timer _nonEmptyOutputQueue;
    boost::asio::deadline_timer _outputDeadline;
    boost::asio::io_context::strand _strand;
    time_t _startTime;
};

typedef std::shared_ptr<tcp_session_tpl> tcp_session_ptr;

//----------------------------------------------------------------------

class udp_broadcaster : public subscriber {
   public:
    udp_broadcaster(boost::asio::io_service& io_service,
                    const boost::asio::ip::udp::endpoint& broadcast_endpoint);

    inline boost::asio::ip::udp::socket& getSocket() { return socket_; }
    void sendPacket(const WorldPacket& p);

   private:
    boost::asio::ip::udp::socket socket_;
};

};  // namespace Divide
#endif
