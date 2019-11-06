#include "stdafx.h"

#include "Headers/Server.h"
#include "Headers/Session.h"

#include "Networking/Headers/ASIO.h"

using namespace boost::asio;

namespace Divide {

Server::Server()
    : thread_(),
      acceptor_(nullptr)
{
}

Server::~Server()
{
    close();
    delete acceptor_;
}

void Server::close() {
    if (!thread_) {
        return; // stopped
    }

    acceptor_->close();
    io_service_.stop();
    thread_->join();
    thread_.reset();
}
void Server::init(U16 port, const stringImpl& broadcast_endpoint_address, bool debugOutput) {
    if (thread_) {
        return;
    }

    _debugOutput = debugOutput;
    try {
        tcp::endpoint listen_endpoint(tcp::v4(), port);
        udp::endpoint broadcast_endpoint(
            boost::asio::ip::address::from_string(broadcast_endpoint_address.c_str()),
            port);
        acceptor_ = new tcp::acceptor(io_service_, listen_endpoint);
        subscriber_ptr bc(new udp_broadcaster(io_service_, broadcast_endpoint));
        _channel.join(bc);
        tcp_session_ptr new_session(new Session(io_service_, _channel));

        acceptor_->async_accept(
            new_session->getSocket(),
            std::bind(&Server::handle_accept, this, new_session, std::placeholders::_1));
        std::auto_ptr<boost::asio::io_service::work> work(
            new boost::asio::io_service::work(io_service_));
        
        thread_.reset(new std::thread([this]() {
            io_service_.run();
        }));

    } catch (std::exception& e) {
        ASIO::LOG_PRINT((stringImpl("SERVER: ") + e.what()).c_str(), true);
    }
}

void Server::handle_accept(tcp_session_ptr session, const boost::system::error_code& ec) {
    if (!ec) {
        if (_debugOutput) {
            ASIO::LOG_PRINT("New TCP session accepted");
        }
        session->start();

        tcp_session_ptr new_session(new Session(io_service_, _channel));

        acceptor_->async_accept(
            new_session->getSocket(),
            std::bind(&Server::handle_accept, this, new_session, std::placeholders::_1));
    } else {
        std::stringstream ss;
        ss << "ERROR: " << ec;
        ASIO::LOG_PRINT(ss.str().c_str());
    }
}

};  // namespace Divide
