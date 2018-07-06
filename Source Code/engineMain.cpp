#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"

/// Comment this out to show the debug console
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
namespace Divide {

int engineMain(int argc, char** argv) {
    std::ofstream stdConsole(OUTPUT_LOG_FILE,
                             std::ofstream::out | std::ofstream::trunc);
    std::ofstream errConsole(ERROR_LOG_FILE,
                             std::ofstream::out | std::ofstream::trunc);
    std::cout.rdbuf(stdConsole.rdbuf());
    std::cerr.rdbuf(errConsole.rdbuf());

    // Initialize our application based on XML configuration. Error codes are
    // always less than 0
    ErrorCode returnCode =
        Application::getInstance().initialize("main.xml", argc, argv);

    if (returnCode == ErrorCode::NO_ERR) {
        Application::getInstance().run();
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

    return to_int(returnCode);
}
};