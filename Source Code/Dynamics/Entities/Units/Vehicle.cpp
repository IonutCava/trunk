#include "Headers/Vehicle.h"

Vehicle::Vehicle(SceneGraphNode* const node) : Unit(Unit::UNIT_TYPE_VEHICLE, node)
{
	_playerControlled = false;
}

Vehicle::~Vehicle()
{
}


void Vehicle::setVehicleTypeMask(U8 mask){
	assert((mask & ~(VEHICLE_TYPE_PLACEHOLDER-1)) == 0);
	_vehicleTypeMask |= static_cast<VEHICLE_TYPES>(mask);
}

bool Vehicle::checkVehicleMask(VEHICLE_TYPES type) const {
	return (_vehicleTypeMask & type) == type ? false : true;
}