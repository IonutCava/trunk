#include "Headers/AESOPActions.h"

namespace AI {

    MoveAction::MoveAction(std::string name, float cost) : AESOPAction(2, name, cost)
    {
    }

    RunAction::RunAction(std::string name, float cost) : AESOPAction(2, name, cost)
    {
    }

    KillAction::KillAction(std::string name, float cost) : AESOPAction(2, name, cost)
    {
    }

    CapturePointAction::CapturePointAction(std::string name, float cost) : AESOPAction(2, name, cost)
    {
    }

    DieAction::DieAction(std::string name, float cost) : AESOPAction(2, name, cost)
    {
    }
};