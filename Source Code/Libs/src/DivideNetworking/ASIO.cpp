#ifndef OPCODE_ENUM
#define OPCODE_ENUM OPcodes
#endif
// There doesn't seem to be any other way to turn this off such that the
// presence of
// the user-defined operator,() below doesn't cause spurious warning all over
// the place,
// so unconditionally turn it off.
#if defined(_MSC_VER)
#pragma warning(push)
// user defined binary operator ',' exists but no overload could convert all
// operands, default built-in binary operator ',' used
#pragma warning(disable : 4913)
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#//pragma GCC diagnostic ignored "-Wall"
#endif

#include "OPCodesTpl.h"
#include "ASIO.h"
#include "Client.h"
#include "Core/Math/Headers/MathHelper.h"
#include <boost/archive/text_iarchive.hpp>

namespace Divide {

ASIO::ASIO() : _connected(false), _debugOutput(true), _localClient(nullptr) {}

ASIO::~ASIO() {
    _work.reset();
    _thread->join();
    _localClient->stop();
    io_service_.stop();
    delete _localClient;
    _localClient = nullptr;
}

void ASIO::disconnect() {
    if (!_connected) return;
    WorldPacket p(CMSG_REQUEST_DISCONNECT);
    p << _localClient->getSocket().local_endpoint().address().to_string();
    sendPacket(p);
}

void ASIO::init(const stringImpl& address, const stringImpl& port) {
    try {
        tcp::resolver res(io_service_);
        _localClient = new Client(this, io_service_, _debugOutput);
        _work.reset(new boost::asio::io_service::work(io_service_));
        _localClient->start(
            res.resolve(tcp::resolver::query(address.c_str(), port.c_str())));
        _thread = new std::thread([&] { io_service_.run(); });
        io_service_.poll();
        _connected = true;
    } catch (std::exception& e) {
        if (_debugOutput) {
            stringImpl msg("ASIO Exception: ");
            msg.append(e.what());
            std::cout << msg.c_str() << std::endl;
        }
    }
}

void ASIO::connect(const stringImpl& address, const stringImpl& port) {
    if (!_connected) {
        init(address, port);
    }
}

bool ASIO::isConnected() const { return _connected; }

void ASIO::close() {
    _localClient->stop();
    _connected = false;
}

void ASIO::sendPacket(WorldPacket& p) const {
    if (!_connected) return;

    _localClient->sendPacket(p);

    if (_debugOutput) {
        std::cout << "ASIO: sent opcode [ 0x"
                  << std::to_string(p.getOpcode()) << "]" << std::endl;
    }
}

void ASIO::toggleDebugOutput(const bool debugOutput) {
    _debugOutput = debugOutput;
    _localClient->toggleDebugOutput(_debugOutput);
}

};  // namespace Divide

#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif