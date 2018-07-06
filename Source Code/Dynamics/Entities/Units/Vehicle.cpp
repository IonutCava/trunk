#include "Headers/Vehicle.h"

namespace Divide {

Vehicle::Vehicle(SceneGraphNode_ptr node)
    : Unit(Unit::UnitType::UNIT_TYPE_VEHICLE),
      _vehicleTypeMask(0)
{
    _playerControlled = false;
}

Vehicle::~Vehicle()
{
}

void Vehicle::setVehicleTypeMask(U32 mask) {
    assert((mask &
            ~(to_const_uint(VehicleType::COUNT) - 1)) == 0);
    _vehicleTypeMask = mask;
}

bool Vehicle::checkVehicleMask(VehicleType type) const {
    return (_vehicleTypeMask & to_uint(type)) == to_uint(type) ? false
                                                                         : true;
}
};