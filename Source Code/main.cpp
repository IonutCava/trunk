#include "stdafx.h"

#include "engineMain.h"

#if defined(_WIN32)
#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#endif

int main(int argc, char **argv) { 
    Divide::U64 callCount = 0;
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);

    const auto started = std::chrono::high_resolution_clock::now();

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
    const auto done = std::chrono::high_resolution_clock::now();

    std::cout << "Divide engine shutdown after "
              << callCount
              << " engine steps. Total time: "
              << std::chrono::duration_cast<std::chrono::seconds>(done - started).count() 
              << " seconds."
              << std::endl;

    return engine.errorCode();
}
