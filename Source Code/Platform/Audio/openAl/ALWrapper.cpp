#include "Headers/ALWrapper.h"

#include <al.h>
#include <alc.h>

namespace Divide {

ErrorCode OpenAL_API::initAudioAPI() {
    // Initialization
    ALCdevice* device = alcOpenDevice(NULL);  // select the "preferred device"
    if (device) {
        ALCcontext* context = alcCreateContext(device, NULL);
        alcMakeContextCurrent(context);
    }
    // Check for EAX 2.0 support
    // ALboolean g_bEAX = alIsExtensionPresent("EAX2.0");
    // Generate Buffers
    alGetError();  // clear error code
    alGenBuffers(MAX_SOUND_BUFFERS, buffers);
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        return ErrorCode::OAL_INIT_ERROR;
    }
    // Clear Error Code (so we can catch any new errors)
    alGetError();
    return ErrorCode::OAL_INIT_ERROR;
}

void OpenAL_API::closeAudioAPI() {}

void OpenAL_API::playSound(const AudioDescriptor_ptr& sound) {}

void OpenAL_API::playMusic(const AudioDescriptor_ptr& music) {}

void OpenAL_API::pauseMusic() {}

void OpenAL_API::stopMusic() {}

void OpenAL_API::stopAllSounds() {}

void OpenAL_API::setMusicVolume(I8 value) {}

void OpenAL_API::setSoundVolume(I8 value) {}
};