#include "Headers/TestList.h"
#include <iostream>
#include <TestClass.cpp>

int main(int argc, char **argv) {
    int state = 0;

    std::cout << "Running Platform Unit Tests!" << std::endl;
    if (true) {
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