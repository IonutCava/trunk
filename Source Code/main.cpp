#include "engineMain.h"
#include <assert.h>

bool staticData = Divide::initStaticData();

int main(int argc, char **argv) { 
    assert(staticData);
    int status = Divide::engineMain(argc, argv); 
    Divide::destroyStaticData();
    return status;
}