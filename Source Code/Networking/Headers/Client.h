/*
Copyright (c) 2018 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#pragma once
#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "WorldPacket.h"

using boost::asio::deadline_timer;
using boost::asio::ip::tcp;

namespace Divide {

class OPCodes;
class ASIO;

class Client {
   public:
    Client(ASIO* asioPointer, boost::asio::io_service& service, bool debugOutput)
        : _asioPointer(asioPointer),
          _stopped(false),
          _debugOutput(debugOutput),
          _socket(service),
          _deadline(service),
          _heartbeatTimer(service)
    {
    }

    // Start:: Called by the user of the client class to initiate the connection
    // process.
    // The endpoint iterator will have been obtained using a tcp::resolver.
    // Stop:: This function terminates all the actors to shut down the
    // connection. It
    // may be called by the user of the client class, or by the class itself in
    // response to graceful termination or an unrecoverable error.

    void start(tcp::resolver::iterator endpoint_iter);
    void stop();

    inline tcp::socket& getSocket() { return _socket; }

    // Packet I/O
    bool sendPacket(WorldPacket& p);
    void receivePacket(WorldPacket& p);

    void toggleDebugOutput(bool debugOutput) { _debugOutput = debugOutput; }

   private:
    // Connection
    void start_connect(tcp::resolver::iterator endpoint_iter);
    void handle_connect(const boost::system::error_code& ec,
                        tcp::resolver::iterator endpoint_iter);

    // Read
    void start_read();
    void handle_read_body(const boost::system::error_code& ec,
                          size_t bytes_transfered);
    void handle_read_packet(const boost::system::error_code& ec,
                            size_t bytes_transfered);
    // File Input
    void receiveFile();
    void handle_read_file(const boost::system::error_code& ec,
                          size_t bytes_transfered);

    // Write
    void start_write();
    void handle_write(const boost::system::error_code& ec);
    void handle_read_file_content(const boost::system::error_code& err,
                                  std::size_t bytes_transfered);

    // Timers
    void check_deadline();

   private:
    bool _stopped, _debugOutput;
    tcp::socket _socket;
    size_t _header;
    boost::asio::streambuf _inputBuffer;
    deadline_timer _deadline;
    deadline_timer _heartbeatTimer;
    std::deque<WorldPacket> _packetQueue;

    // File Data
    std::ofstream _outputFile;
    boost::asio::streambuf _requestBuf;
    size_t _fileSize;
    boost::array<char, 1024> _buf;
    ASIO* _asioPointer;
};

};  // namespace Divide
#endif
