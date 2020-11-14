#include "stdafx.h"

#include "Headers/AudioSensor.h"

namespace Divide {
using namespace AI;

AudioSensor::AudioSensor(AIEntity* const parentEntity)
    : Sensor(parentEntity, SensorType::AUDIO_SENSOR)
{
}

void AudioSensor::update(const U64 deltaTimeUS) {
    ACKNOWLEDGE_UNUSED(deltaTimeUS);
}

}  // namespace Divide