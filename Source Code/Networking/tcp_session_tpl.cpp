#include "Headers/tcp_session_tpl.h"
#include "Headers/OPCodesTpl.h"

#include <boost/archive/text_iarchive.hpp>
#include <fstream>
#include <iostream>

///////////////////////////////////////////////////////////////////////////////////////
//                                     TCP //
///////////////////////////////////////////////////////////////////////////////////////

namespace Divide {

tcp_session_tpl::tcp_session_tpl(boost::asio::io_service& io_service,
                                 channel& ch)
    : _startTime(time(nullptr)),
      _channel(ch),
      _socket(io_service),
      _inputDeadline(io_service),
      _nonEmptyOutputQueue(io_service),
      _outputDeadline(io_service),
      _strand(io_service) {
    _inputDeadline.expires_at(boost::posix_time::pos_infin);
    _outputDeadline.expires_at(boost::posix_time::pos_infin);
    _nonEmptyOutputQueue.expires_at(boost::posix_time::pos_infin);
}

void tcp_session_tpl::start() {
    _channel.join(shared_from_this());

    start_read();

    _inputDeadline.async_wait(
        _strand.wrap(boost::bind(&tcp_session_tpl::check_deadline,
                                 shared_from_this(), &_inputDeadline)));

    await_output();

    _outputDeadline.async_wait(
        _strand.wrap(boost::bind(&tcp_session_tpl::check_deadline,
                                 shared_from_this(), &_outputDeadline)));
}

void tcp_session_tpl::stop() {
    _channel.leave(shared_from_this());

    _socket.close();
    _inputDeadline.cancel();
    _nonEmptyOutputQueue.cancel();
    _outputDeadline.cancel();
}

bool tcp_session_tpl::stopped() const { return !_socket.is_open(); }

void tcp_session_tpl::sendPacket(const WorldPacket& p) {
    _outputQueue.push_back(p);
    _nonEmptyOutputQueue.expires_at(boost::posix_time::neg_infin);
}

void tcp_session_tpl::sendFile(const stringImpl& name) {
    _outputFileQueue.push_back(name);
}

void tcp_session_tpl::start_read() {
    _header = 0;
    _inputBuffer.consume(_inputBuffer.size());
    _inputDeadline.expires_from_now(boost::posix_time::seconds(30));
    boost::asio::async_read(
        _socket, boost::asio::buffer(&_header, sizeof(_header)),
        _strand.wrap(
            boost::bind(&tcp_session_tpl::handle_read_body, shared_from_this(),
                        _1, boost::asio::placeholders::bytes_transferred)));
}

void tcp_session_tpl::handle_read_body(const boost::system::error_code& ec,
                                       size_t bytes_transfered) {
    ACKNOWLEDGE_UNUSED(bytes_transfered);

    if (stopped()) {
        return;
    }

    if (!ec) {
        _inputDeadline.expires_from_now(boost::posix_time::seconds(30));
        boost::asio::async_read(
            _socket, _inputBuffer.prepare(_header),
            _strand.wrap(boost::bind(
                &tcp_session_tpl::handle_read_packet, shared_from_this(), _1,
                boost::asio::placeholders::bytes_transferred)));
    } else {
        stop();
    }
}

void tcp_session_tpl::handle_read_packet(const boost::system::error_code& ec,
                                         size_t bytes_transfered) {
    ACKNOWLEDGE_UNUSED(bytes_transfered);

    if (stopped()) {
        return;
    }

    if (!ec) {
        _inputBuffer.commit(_header);
        std::cout << "Buffer size: " << _header << std::endl;
        std::istream is(&_inputBuffer);
        WorldPacket packet;
        try {
            boost::archive::text_iarchive ar(is);
            ar& packet;
        } catch (std::exception& e) {
            std::cout << e.what() << std::endl;
        }

        handlePacket(packet);
        start_read();
    } else {
        stop();
    }
}

void tcp_session_tpl::start_write() {
    if (_outputQueue.empty()) await_output();

    boost::asio::streambuf buf;
    std::ostream os(&buf);

    // Set a deadline for the write operation.
    _outputDeadline.expires_from_now(boost::posix_time::seconds(30));

    WorldPacket& p = _outputQueue.front();
    boost::archive::text_oarchive ar(os);
    ar& p;  // Archive the packet

    size_t header = buf.size();
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(boost::asio::buffer(&header, sizeof(header)));
    buffers.push_back(buf.data());
    // Start an asynchronous operation to send a message.
    if (p.opcode() == OPCodes::SMSG_SEND_FILE) {
        boost::asio::async_write(
            _socket, buffers,
            _strand.wrap(boost::bind(&tcp_session_tpl::handle_write_file,
                                     shared_from_this(), _1)));
    } else {
        boost::asio::async_write(
            _socket, buffers,
            _strand.wrap(boost::bind(&tcp_session_tpl::handle_write,
                                     shared_from_this(), _1)));
    }
}
void tcp_session_tpl::handle_write_file(const boost::system::error_code& ec) {
    ACKNOWLEDGE_UNUSED(ec);

    boost::asio::streambuf request_;
    stringImpl filePath = _outputFileQueue.front();
    std::ifstream source_file;
    source_file.open(filePath.c_str(),
                     std::ios_base::binary | std::ios_base::ate);
    if (!source_file) {
        std::cout << "failed to open " << filePath << std::endl;
        return;
    }
    size_t file_size = sizeof(source_file);  //.tellg();
    source_file.seekg(0);
    // first send file name and file size to server
    std::ostream request_stream(&request_);
    request_stream << filePath << "\n" << file_size << "\n\n";
    std::cout << "request size:" << request_.size() << std::endl;

    // Start an asynchronous resolve to translate the server and service names
    // into a list of endpoints.
    _outputFileQueue.pop_front();
    boost::asio::async_write(
        _socket, request_,
        _strand.wrap(boost::bind(&tcp_session_tpl::handle_write,
                                 shared_from_this(), _1)));
}

void tcp_session_tpl::handle_write(const boost::system::error_code& ec) {
    if (stopped()) return;

    if (!ec) {
        _outputQueue.pop_front();
        await_output();
    } else {
        stop();
    }
}

void tcp_session_tpl::await_output() {
    if (stopped()) return;

    if (_outputQueue.empty()) {
        if (_outputQueue.empty()) {
            _nonEmptyOutputQueue.expires_at(boost::posix_time::pos_infin);
            _nonEmptyOutputQueue.async_wait(boost::bind(
                &tcp_session_tpl::await_output, shared_from_this()));
        }
    } else {
        start_write();
    }
}

void tcp_session_tpl::check_deadline(boost::asio::deadline_timer* deadline) {
    if (stopped()) return;

    // Check whether the deadline has passed. We compare the deadline against
    // the current time since a new asynchronous operation may have moved the
    // deadline before this actor had a chance to run.
    if (deadline->expires_at() <= 
        boost::asio::deadline_timer::traits_type::now()) {
        // The deadline has passed. Stop the session. The other actors will
        // terminate as soon as possible.
        stop();
    } else {
        // Put the actor back to sleep.
        deadline->async_wait(boost::bind(&tcp_session_tpl::check_deadline,
                                         shared_from_this(), deadline));
    }
}

///////////////////////////////////////////////////////////////////////////////////////
//                                     UDP                                           //
///////////////////////////////////////////////////////////////////////////////////////

udp_broadcaster::udp_broadcaster(boost::asio::io_service& io_service,
                      const boost::asio::ip::udp::endpoint& broadcast_endpoint)
    : socket_(io_service) {
    socket_.connect(broadcast_endpoint);
}

void udp_broadcaster::sendPacket(const WorldPacket& p) {
    boost::asio::streambuf buf;
    std::ostream os(&buf);
    boost::archive::text_oarchive ar(os);
    ar& p;  // Archive the packet

    size_t header = buf.size();
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(boost::asio::buffer(&header, sizeof(header)));
    buffers.push_back(buf.data());
    boost::system::error_code ignored_ec;
    socket_.send(buffers, 0, ignored_ec);
}

void tcp_session_tpl::handlePacket(WorldPacket& p) {
    switch (p.opcode()) {
        case OPCodes::MSG_HEARTBEAT:
            std::cout << "Received [ MSG_HEARTBEAT ]" << std::endl;
            HandleHeartBeatOpCode(p);
            break;
        case OPCodes::CMSG_PING:
            std::cout << "Received [ CMSG_PING ]" << std::endl;
            HandlePingOpCode(p);
            break;
        case OPCodes::CMSG_REQUEST_DISCONNECT:
            HandleDisconnectOpCode(p);
            break;
        case OPCodes::CMSG_ENTITY_UPDATE:
            HandleEntityUpdateOpCode(p);
            break;
        default:
            DIVIDE_UNEXPECTED_CALL("Unknown network message!");
            break;
    }
}

void tcp_session_tpl::HandleHeartBeatOpCode(WorldPacket& p) {
    ACKNOWLEDGE_UNUSED(p);

    WorldPacket r(OPCodes::MSG_HEARTBEAT);
    std::cout << "Sending  [ MSG_HEARTBEAT]" << std::endl;
    r << (I8)0;
    sendPacket(r);
}

void tcp_session_tpl::HandlePingOpCode(WorldPacket& p) {
    F32 time = 0;
    p >> time;
    std::cout << "Sending  [ SMSG_PONG ] with data: " << time << std::endl;
    WorldPacket r(OPCodes::SMSG_PONG);
    r << time;
    sendPacket(r);
}

void tcp_session_tpl::HandleDisconnectOpCode(WorldPacket& p) {
    stringImpl client;
    p >> client;
    std::cout << "Received [ CMSG_REQUEST_DISCONNECT ] from: [ " << client << " ]" << std::endl;
    WorldPacket r(OPCodes::SMSG_DISCONNECT);
    r << (U8)0;  // this will be the error code returned after safely saving
                 // client
    sendPacket(r);
}

void tcp_session_tpl::HandleEntityUpdateOpCode(WorldPacket& p) {
    std::cout << "Received [ CMSG_ENTITY_UPDATE ] !" << std::endl;
    UpdateEntities(p);
}

};  // namespace Divide
