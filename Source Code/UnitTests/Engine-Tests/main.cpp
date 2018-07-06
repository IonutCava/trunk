#include "stdafx.h"

#include "Headers/Defines.h"
#include "Core/Headers/Console.h"
#include "Platform/Headers/PlatformDefines.h"

#include <iostream>

void PreparePlatform() {
    static bool s_platformInit = false;
    if (!s_platformInit) {
        Divide::PlatformInit(0, nullptr);
        s_platformInit = true;
    }
}

int main(int argc, char **argv) {

    //Divide::Console::toggle(false);

    std::cout << "Running Engine Unit Tests!" << std::endl;

    int state = 0;
    if (TEST_HAS_FAILED) {
        std::cout << "Errors detected!" << std::endl;
        state = -1;
    } else {
        std::cout << "No errors detected!" << std::endl;
    }

    if (argc == 1) {
        system("pause");
    }

    return state;
}

