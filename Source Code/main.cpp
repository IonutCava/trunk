#include "HotReloading/Headers/HotReloading.h"

#include "engineMain.h"

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