#include "stdafx.h"

#include "Headers/Defines.h"

#include "Core/Headers/Console.h"
#include "Platform/Headers/PlatformDefines.h"

#include <iostream>

bool PreparePlatform() {
    static Divide::ErrorCode err = Divide::ErrorCode::PLATFORM_INIT_ERROR;
    if (err != Divide::ErrorCode::NO_ERR) {
        Divide::PlatformClose();
        vectorImpl<char*> args = { "disableCopyright" };
        err = Divide::PlatformInit(static_cast<int>(args.size()), args.data());
        if (err != Divide::ErrorCode::NO_ERR) {
            std::cout << "Platform error code: " << static_cast<int>(err) << std::endl;
        }
    }

    return err == Divide::ErrorCode::NO_ERR;
}

int main(int argc, char **argv) {

    std::cout << "Running Engine Unit Tests!" << std::endl;

    int state = 0;
    if (TEST_HAS_FAILED) {
        std::cout << "Errors detected!" << std::endl;
        state = -1;
    } else {
        std::cout << "No errors detected!" << std::endl;
    }

    Divide::PlatformClose();

    if (argc == 1) {
        system("pause");
    }

    return state;
}

