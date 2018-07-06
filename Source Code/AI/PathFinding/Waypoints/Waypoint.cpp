#include "stdafx.h"

#include "Headers/Waypoint.h"
#include "Core/Headers/Console.h"

namespace Divide {
namespace Navigation {

Waypoint::Waypoint()
{
    STUBBED("ToDo: add accessors to the Waypoint class! -Ionut")
    _id = 0xFFFFFFFF;
    _time = 0;
}

Waypoint::~Waypoint() {}
};
};