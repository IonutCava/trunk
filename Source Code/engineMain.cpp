#include "engineMain.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"

#include <iostream>

namespace Divide {

static void out_of_memory()
{
    assert(0);
}

bool initStaticData() {
    bool status = PlatformInitStatic();
    if (status) {
        status = Application::initStaticData();
    }
    return status;
}

int engineMain(int argc, char** argv) {
    std::set_new_handler(out_of_memory);

    ErrorCode returnCode = ErrorCode::PLATFORM_INIT_ERROR;
    if (PlatformInit()) {
        std::ofstream stdConsole(OUTPUT_LOG_FILE,
                                 std::ofstream::out | std::ofstream::trunc);
        std::ofstream errConsole(ERROR_LOG_FILE,
                                 std::ofstream::out | std::ofstream::trunc);
        std::cout.rdbuf(stdConsole.rdbuf());
        std::cerr.rdbuf(errConsole.rdbuf());

        // Initialize our application based on XML configuration. Error codes are
        // always less than 0
        returnCode = Application::instance().initialize("main.xml", argc, argv);

        if (returnCode == ErrorCode::NO_ERR) {
            Application::instance().run();
        } else {
            // If any error occurred, close the application as details should
            // already be logged
            Console::errorfn("System failed to initialize properly. Error [ %s ] ",
                             getErrorCodeName(returnCode));
        }

        // Stop our application
        // When the application is deleted, the last kernel used is deleted as well
        Application::destroyInstance();

        std::cout << std::endl;
        std::cerr << std::endl;

        if (returnCode == ErrorCode::NO_ERR) {
            if (!PlatformClose()) {
                returnCode = ErrorCode::PLATFORM_CLOSE_ERROR;
            }
        }
    }

    return to_int(returnCode);
}
};
