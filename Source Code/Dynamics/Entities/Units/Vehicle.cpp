#include "Headers/Vehicle.h"

namespace Divide {

Vehicle::Vehicle(SceneGraphNode& node)
    : Unit(Unit::UnitType::UNIT_TYPE_VEHICLE, node) {
    _playerControlled = false;
}

Vehicle::~Vehicle() {}

void Vehicle::setVehicleTypeMask(U32 mask) {
    assert((mask &
            ~(to_uint(VehicleType::COUNT) - 1)) == 0);
    _vehicleTypeMask = mask;
}

bool Vehicle::checkVehicleMask(VehicleType type) const {
    return (_vehicleTypeMask & to_uint(type)) == to_uint(type) ? false
                                                                         : true;
}
};