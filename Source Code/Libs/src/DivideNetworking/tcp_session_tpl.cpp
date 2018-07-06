#include "tcp_session_tpl.h"
#include <fstream>

#include "OPCodesTpl.h"

#include <boost/archive/text_iarchive.hpp>
///////////////////////////////////////////////////////////////////////////////////////
//                                     TCP //
///////////////////////////////////////////////////////////////////////////////////////

namespace Divide {

tcp_session_tpl::tcp_session_tpl(boost::asio::io_service& io_service,
                                 channel& ch)
    : start_time(time(nullptr)),
      channel_(ch),
      socket_(io_service),
      input_deadline_(io_service),
      non_empty_output_queue_(io_service),
      output_deadline_(io_service),
      _strand(io_service) {
    input_deadline_.expires_at(boost::posix_time::pos_infin);
    output_deadline_.expires_at(boost::posix_time::pos_infin);
    non_empty_output_queue_.expires_at(boost::posix_time::pos_infin);
}

void tcp_session_tpl::start() {
    channel_.join(shared_from_this());

    start_read();

    input_deadline_.async_wait(
        _strand.wrap(boost::bind(&tcp_session_tpl::check_deadline,
                                 shared_from_this(), &input_deadline_)));

    await_output();

    output_deadline_.async_wait(
        _strand.wrap(boost::bind(&tcp_session_tpl::check_deadline,
                                 shared_from_this(), &output_deadline_)));
}

void tcp_session_tpl::stop() {
    channel_.leave(shared_from_this());

    socket_.close();
    input_deadline_.cancel();
    non_empty_output_queue_.cancel();
    output_deadline_.cancel();
}

bool tcp_session_tpl::stopped() const { return !socket_.is_open(); }

void tcp_session_tpl::sendPacket(const WorldPacket& p) {
    output_queue_.push_back(p);
    non_empty_output_queue_.expires_at(boost::posix_time::neg_infin);
}

void tcp_session_tpl::sendFile(const stringImpl& name) {
    output_file_queue_.push_back(name);
}

void tcp_session_tpl::start_read() {
    header = 0;
    input_buffer_.consume(input_buffer_.size());
    input_deadline_.expires_from_now(boost::posix_time::seconds(30));
    boost::asio::async_read(
        socket_, boost::asio::buffer(&header, sizeof(header)),
        _strand.wrap(
            boost::bind(&tcp_session_tpl::handle_read_body, shared_from_this(),
                        _1, boost::asio::placeholders::bytes_transferred)));
}

void tcp_session_tpl::handle_read_body(const boost::system::error_code& ec,
                                       size_t bytes_transfered) {
    if (stopped()) return;

    if (!ec) {
        input_deadline_.expires_from_now(boost::posix_time::seconds(30));
        boost::asio::async_read(
            socket_, input_buffer_.prepare(header),
            _strand.wrap(boost::bind(
                &tcp_session_tpl::handle_read_packet, shared_from_this(), _1,
                boost::asio::placeholders::bytes_transferred)));
    } else {
        stop();
    }
}

void tcp_session_tpl::handle_read_packet(const boost::system::error_code& ec,
                                         size_t bytes_transfered) {
    if (stopped()) return;

    if (!ec) {
        input_buffer_.commit(header);
        std::cout << "Buffer size: " << header << std::endl;
        std::istream is(&input_buffer_);
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
    if (output_queue_.empty()) await_output();

    boost::asio::streambuf buf;
    std::ostream os(&buf);

    // Set a deadline for the write operation.
    output_deadline_.expires_from_now(boost::posix_time::seconds(30));

    WorldPacket& p = output_queue_.front();
    boost::archive::text_oarchive ar(os);
    ar& p;  // Archive the packet

    size_t header = buf.size();
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(boost::asio::buffer(&header, sizeof(header)));
    buffers.push_back(buf.data());
    // Start an asynchronous operation to send a message.
    if (p.getOpcode() == OPCodes::SMSG_SEND_FILE) {
        boost::asio::async_write(
            socket_, buffers,
            _strand.wrap(boost::bind(&tcp_session_tpl::handle_write_file,
                                     shared_from_this(), _1)));
    } else {
        boost::asio::async_write(
            socket_, buffers,
            _strand.wrap(boost::bind(&tcp_session_tpl::handle_write,
                                     shared_from_this(), _1)));
    }
}
void tcp_session_tpl::handle_write_file(const boost::system::error_code& ec) {
    boost::asio::streambuf request_;
    stringImpl filePath = output_file_queue_.front();
    std::ifstream source_file;
    source_file.open(filePath.c_str(),
                     std::ios_base::binary | std::ios_base::ate);
    if (!source_file) {
        std::cout << "failed to open " << filePath.c_str() << std::endl;
        return;
    }
    size_t file_size = sizeof(source_file);  //.tellg();
    source_file.seekg(0);
    // first send file name and file size to server
    std::ostream request_stream(&request_);
    request_stream << filePath.c_str() << "\n" << file_size << "\n\n";
    std::cout << "request size:" << request_.size() << std::endl;

    // Start an asynchronous resolve to translate the server and service names
    // into a list of endpoints.
    output_file_queue_.pop_front();
    boost::asio::async_write(
        socket_, request_,
        _strand.wrap(boost::bind(&tcp_session_tpl::handle_write,
                                 shared_from_this(), _1)));
}

void tcp_session_tpl::handle_write(const boost::system::error_code& ec) {
    if (stopped()) return;

    if (!ec) {
        output_queue_.pop_front();
        await_output();
    } else {
        stop();
    }
}

void tcp_session_tpl::await_output() {
    if (stopped()) return;

    if (output_queue_.empty()) {
        if (output_queue_.empty()) {
            non_empty_output_queue_.expires_at(boost::posix_time::pos_infin);
            non_empty_output_queue_.async_wait(boost::bind(
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
//                                     UDP //
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

};  // namespace Divide
