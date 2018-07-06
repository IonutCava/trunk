#include "config.h"

#include "engineMain.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"

#include <iostream>

namespace Divide {

namespace {
    void out_of_memory()
    {
        DIVIDE_ASSERT(false, "Out of memory!");
    }
};

class StreamBuffer {
public:
    StreamBuffer(const char* filename)
        : _buf(std::ofstream(filename, std::ofstream::out | std::ofstream::trunc))
    {
    }

    inline std::ofstream& buffer() {
        return _buf;
    }

private:
    std::ofstream _buf;
};

Engine::Engine() :_app(Application::instance()),
                  _errorCode(0)
{
    std::set_new_handler(out_of_memory);
    _outputStreams[0] = new StreamBuffer(OUTPUT_LOG_FILE);
    _outputStreams[1] = new StreamBuffer(ERROR_LOG_FILE);
    std::cout.rdbuf(_outputStreams[0]->buffer().rdbuf());
    std::cerr.rdbuf(_outputStreams[1]->buffer().rdbuf());
}

Engine::~Engine()
{
    std::cout << std::endl;
    std::cerr << std::endl;
    delete _outputStreams[0];
    delete _outputStreams[1];
}

bool Engine::init(int argc, char** argv) {
    ErrorCode err = ErrorCode::NO_ERR;

    if (!PlatformInit()) {
        err = ErrorCode::PLATFORM_INIT_ERROR;
    } else {
        // Start our application based on XML configuration.
        // If it fails to start, it should automatically clear up all of its data
        err = _app.start("main.xml", argc, argv);
        if (err != ErrorCode::NO_ERR) {
            // If any error occurred, close the application as details should
            // already be logged
            Console::errorfn("System failed to initialize properly. Error [ %s ] ",
                             getErrorCodeName(err));
        }

    }

    _errorCode = to_int(err);

    return err == ErrorCode::NO_ERR;
}

void Engine::shutdown() {
    _app.stop();

    if (!PlatformClose()) {
        _errorCode = to_int(ErrorCode::PLATFORM_CLOSE_ERROR);
    }
}

bool Engine::step() {
    assert(_errorCode == 0);
        
    return _app.step();
}

int Engine::errorCode() const {
    return to_int(_errorCode);
}

};
