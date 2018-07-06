#include "Headers/AESOPActionInterface.h"

namespace AI {

AESOPAction::AESOPAction(U32 numParams, std::string name, float cost) : Aesop::Action(name, cost)
{
    mNumParams = numParams;
}

AESOPAction::~AESOPAction()
{
}

}; //namespace AI