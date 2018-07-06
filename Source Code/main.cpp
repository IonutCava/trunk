#include "stdafx.h"

#include "HotReloading/Headers/HotReloading.h"

#include "engineMain.h"

#if defined(_WIN32)
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#endif

int main(int argc, char **argv) { 
    // Create a new engine instance
    Divide::Engine engine;
    // Start the engine
    if (engine.init(argc, argv)) {
        // Step the entire application
        while(engine.step())
        {
        }
    }
    // Stop the engine
    engine.shutdown();

    return engine.errorCode();
}
