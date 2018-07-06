#include "Headers/AudioSensor.h"

namespace Divide {
using namespace AI;

AudioSensor::AudioSensor(AIEntity* const parentEntity)
    : Sensor(parentEntity, AUDIO_SENSOR) {}

void AudioSensor::update(const U64 deltaTime) {}

};  // namespace Divide