#ifndef OPCODE_ENUM
#define OPCODE_ENUM OPcodes
#endif

#if defined(_MSC_VER)
#pragma warning(push)
// user defined binary operator ',' exists but no overload could convert all
// operands, default built-in binary operator ',' used
#pragma warning(disable : 4913)
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#//pragma GCC diagnostic ignored "-Wall"
#endif

#include "Client.h"
#include "ASIO.h"
#include "OPCodesTpl.h"

#include <boost/archive/text_iarchive.hpp>

namespace Divide {

void Client::sendPacket(WorldPacket& p) {
    _packetQueue.push_back(p);
    heartbeat_timer_.expires_at(boost::posix_time::neg_infin);
}

void Client::receivePacket(WorldPacket& p) { _asioPointer->handlePacket(p); }

void Client::start(tcp::resolver::iterator endpoint_iter) {
    start_connect(endpoint_iter);
    deadline_.async_wait(boost::bind(&Client::check_deadline, this));
}

void Client::stop() {
    stopped_ = true;
    socket_.close();
    deadline_.cancel();
    heartbeat_timer_.cancel();
}

void Client::start_read() {
    // Set a deadline for the read operation.
    deadline_.expires_from_now(boost::posix_time::seconds(30));
    header = 0;
    input_buffer_.consume(input_buffer_.size());
    // Start an asynchronous operation to read a newline-delimited message.
    boost::asio::async_read(
        socket_, boost::asio::buffer(&header, sizeof(header)),
        boost::bind(&Client::handle_read_body, this, _1,
                    boost::asio::placeholders::bytes_transferred));
}

void Client::handle_read_body(const boost::system::error_code& ec,
                              size_t bytes_transfered) {
    if (stopped_) return;

    if (!ec) {
        deadline_.expires_from_now(boost::posix_time::seconds(30));
        boost::asio::async_read(
            socket_, input_buffer_.prepare(header),
            boost::bind(&Client::handle_read_packet, this, _1,
                        boost::asio::placeholders::bytes_transferred));
    } else {
        stop();
    }
}

void Client::handle_read_packet(const boost::system::error_code& ec,
                                size_t bytes_transfered) {
    if (stopped_) return;

    if (!ec) {
        input_buffer_.commit(header);
        std::istream is(&input_buffer_);
        WorldPacket packet;
        try {
            boost::archive::text_iarchive ar(is);
            ar& packet;
        } catch (std::exception& e) {
            if (_debugOutput) {
                std::cout << e.what() << std::endl;
                std::cout << "ASIO: Received invalid packet!" << std::endl;
            }
        }

        if (packet.getOpcode() == OPCodes::SMSG_SEND_FILE) {
            boost::asio::async_read_until(
                socket_, request_buf, "\n\n",
                boost::bind(&Client::handle_read_file, this,
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
        } else {
            receivePacket(packet);
            start_read();
        }
    } else {
        stop();
    }
}

void Client::handle_read_file(const boost::system::error_code& ec,
                              size_t bytes_transfered) {
    std::cout << __FUNCTION__ << "(" << bytes_transfered << ")"
              << ", in_avail=" << request_buf.in_avail()
              << ", size=" << request_buf.size()
              << ", max_size=" << request_buf.max_size() << ".\n";
    std::istream request_stream(&request_buf);
    std::string file_path;
    request_stream >> file_path;
    request_stream >> file_size;
    request_stream.read(buf.c_array(), 2);  // eat the "\n\n"
    std::cout << file_path << " size is " << file_size
              << ", tellg=" << request_stream.tellg() << std::endl;
    size_t pos = file_path.find_last_of('\\');
    if (pos != stringImpl::npos) file_path = file_path.substr(pos + 1);
    output_file.open(file_path.c_str(), std::ios_base::binary);
    if (!output_file) {
        std::cout << "failed to open " << file_path << std::endl;
        return;
    }
    // write extra bytes to file
    do {
        request_stream.read(buf.c_array(), (std::streamsize)buf.size());
        std::cout << __FUNCTION__ << " write " << request_stream.gcount()
                  << " bytes.\n";
        output_file.write(buf.c_array(), request_stream.gcount());
    } while (request_stream.gcount() > 0);

    async_read(socket_, boost::asio::buffer(buf.c_array(), buf.size()),
               boost::bind(&Client::handle_read_file_content, this,
                           boost::asio::placeholders::error,
                           boost::asio::placeholders::bytes_transferred));
}

void Client::handle_read_file_content(const boost::system::error_code& err,
                                      std::size_t bytes_transferred) {
    if (bytes_transferred > 0) {
        output_file.write(buf.c_array(), (std::streamsize)bytes_transferred);
        std::cout << __FUNCTION__ << " recv " << output_file.tellp()
                  << " bytes." << std::endl;
        if (output_file.tellp() >= (std::streamsize)file_size) {
            return;
        }
    }
    if (err) stop();
    start_read();
}

void handle_error(const stringImpl& function_name,
                  const boost::system::error_code& err) {
    std::cout << __FUNCTION__ << " in " << function_name.c_str() << " due to "
              << err << " " << err.message() << std::endl;
}

void Client::start_write() {
    if (stopped_) return;

    if (_packetQueue.empty()) {
        WorldPacket heart(OPCodes::MSG_HEARTBEAT);
        heart << (I8)0;
        _packetQueue.push_back(heart);
    }

    WorldPacket& p = _packetQueue.front();
    boost::asio::streambuf buf;
    std::ostream os(&buf);
    boost::archive::text_oarchive ar(os);
    ar& p;

    size_t header = buf.size();
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(boost::asio::buffer(&header, sizeof(header)));
    buffers.push_back(buf.data());
    boost::asio::async_write(socket_, buffers,
                             boost::bind(&Client::handle_write, this, _1));
}

void Client::handle_write(const boost::system::error_code& ec) {
    if (stopped_) return;

    if (!ec) {
        _packetQueue.pop_front();
        heartbeat_timer_.expires_from_now(boost::posix_time::seconds(2));
        heartbeat_timer_.async_wait(boost::bind(&Client::start_write, this));
    } else {
        if (_debugOutput) {
            std::cout << "Error on packet: " << ec.message() << "\n";
            stop();
        }
    }
}

void Client::check_deadline() {
    if (stopped_) return;

    // Check whether the deadline has passed. We compare the deadline against
    // the current time since a new asynchronous operation may have moved the
    // deadline before this actor had a chance to run.
    if (deadline_.expires_at() <= deadline_timer::traits_type::now()) {
        // The deadline has passed. The socket is closed so that any outstanding
        // asynchronous operations are cancelled.
        socket_.close();

        // There is no longer an active deadline. The expiry is set to positive
        // infinity so that the actor takes no action until a new deadline is
        // set.
        deadline_.expires_at(boost::posix_time::pos_infin);
    }

    // Put the actor back to sleep.
    deadline_.async_wait(boost::bind(&Client::check_deadline, this));
}

void Client::start_connect(tcp::resolver::iterator endpoint_iter) {
    if (endpoint_iter != tcp::resolver::iterator()) {
        if (_debugOutput)
            std::cout << "Trying " << endpoint_iter->endpoint() << "...\n";

        // Set a deadline for the connect operation.
        deadline_.expires_from_now(boost::posix_time::seconds(60));

        // Start the asynchronous connect operation.
        socket_.async_connect(
            endpoint_iter->endpoint(),
            boost::bind(&Client::handle_connect, this, _1, endpoint_iter));
    } else {
        // There are no more endpoints to try. Shut down the client.
        stop();
    }
}

void Client::handle_connect(const boost::system::error_code& ec,
                            tcp::resolver::iterator endpoint_iter) {
    if (stopped_) return;

    // The async_connect() function automatically opens the socket at the start
    // of the asynchronous operation. If the socket is closed at this time then
    // the timeout handler must have run first.
    if (!socket_.is_open()) {
        if (_debugOutput) std::cout << "Connect timed out\n";

        // Try the next available endpoint.
        start_connect(++endpoint_iter);
    }

    // Check if the connect operation failed before the deadline expired.
    else if (ec) {
        if (_debugOutput)
            std::cout << "Connect error: " << ec.message() << "\n";

        // We need to close the socket used in the previous connection attempt
        // before starting a new one.
        socket_.close();

        // Try the next available endpoint.
        start_connect(++endpoint_iter);
    }

    // Otherwise we have successfully established a connection.
    else {
        if (_debugOutput)
            std::cout << "Connected to " << endpoint_iter->endpoint() << "\n";

        // Start the input actor.
        start_read();

        // Start the heartbeat actor.
        start_write();
    }
}

};  // namespace Divide
#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif