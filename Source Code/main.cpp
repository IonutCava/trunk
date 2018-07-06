#include "stdafx.h"

#include "HotReloading/Headers/HotReloading.h"

#include "engineMain.h"

#if defined(_WIN32)
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#endif

int main(int argc, char **argv) { 
    Divide::U64 callCount = 0;

    // Create a new engine instance
    Divide::Engine engine;
    // Start the engine
    if (engine.init(argc, argv)) {
        // Step the entire application
        while(engine.step())
        {
            ++callCount;
        }
    }
    // Stop the engine
    engine.shutdown();

    std::cout << "Divide engine shutdown after " << callCount << "." << std::endl;

    return engine.errorCode();
}
