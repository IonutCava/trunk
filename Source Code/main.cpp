#include "engineMain.h"
#include <assert.h>

bool staticData = Divide::initStaticData();

int main(int argc, char **argv) { 
    assert(staticData);
    return Divide::engineMain(argc, argv); 
}