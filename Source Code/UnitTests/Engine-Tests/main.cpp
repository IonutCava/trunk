#include "Headers/TestList.h"
#include <iostream>

namespace Divide {

I32 Error() {
    return GENERAL_ERROR;
}

U32 runTests() {
    U32 ret = NO_ERROR;
    if ((ret = run_StringTests()) != NO_ERROR) {
        return ret;
    }

    return ret;
}

}; //namespace Divide;

int main(int argc, char **argv) {
    int state = 0;

    std::cout << "Running Engine Unit Tests!" << std::endl;
    if (Divide::runTests() == Divide::NO_ERROR) {
        std::cout << "No errors detected!" << std::endl;
    } else {
        std::cout << "Errors detected!" << std::endl;
        state = -1;
    }

    if (argc == 1) {
        system("pause");
    }

    return state;
}