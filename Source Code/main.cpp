#include "engineMain.h"

#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")

int main(int argc, char **argv) { 
    return Divide::engineMain(argc, argv); 
}