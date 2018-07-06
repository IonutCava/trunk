#include "Headers/SFXDevice.h"

#include "Platform/Audio/fmod/Headers/FmodWrapper.h"
#include "Platform/Audio/sdl_mixer/Headers/SDLWrapper.h"
#include "Platform/Audio/openAl/Headers/ALWrapper.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

SFXDevice::SFXDevice() : _state(true, true, true, true),
                         _API_ID(AudioAPI::COUNT),
                         _api(nullptr)
{
}

SFXDevice::~SFXDevice()
{
}

ErrorCode SFXDevice::initAudioAPI() {
    DIVIDE_ASSERT(_api == nullptr,
                  "SFXDevice error: initAudioAPI called twice!");

    switch (_API_ID) {
        case AudioAPI::FMOD: {
            _api = &FMOD_API::getInstance();
        } break;
        case AudioAPI::OpenAL: {
            _api = &OpenAL_API::getInstance();
        } break;
        case AudioAPI::SDL: {
            _api = &SDL_API::getInstance();
        } break;
        default: {
            Console::errorfn(Locale::get("ERROR_SFX_DEVICE_API"));
            return ErrorCode::SFX_NON_SPECIFIED;
        } break;
    };

    return _api->initAudioAPI();
}

void SFXDevice::closeAudioAPI() {
    DIVIDE_ASSERT(_api != nullptr,
                "SFXDevice error: closeAudioAPI called without init!");

    _api->closeAudioAPI();

    switch (_API_ID) {
        case AudioAPI::FMOD: {
            FMOD_API::destroyInstance();
        } break;
        case  AudioAPI::OpenAL: {
            OpenAL_API::destroyInstance();
        } break;
        case AudioAPI::SDL: {
            SDL_API::destroyInstance();
        } break;
        default: {
        } break; 
    };
}

void SFXDevice::playSound(AudioDescriptor* sound) {
    DIVIDE_ASSERT(_api != nullptr,
                "SFXDevice error: playSound called without init!");

    _api->playSound(sound);
}

void SFXDevice::playMusic(AudioDescriptor* music) {
    DIVIDE_ASSERT(_api != nullptr,
                "SFXDevice error: playMusic called without init!");

    _api->playMusic(music);
}

void SFXDevice::pauseMusic() {
    DIVIDE_ASSERT(_api != nullptr,
                "SFXDevice error: pauseMusic called without init!");

    _api->pauseMusic();
}

void SFXDevice::stopMusic() {
    DIVIDE_ASSERT(_api != nullptr,
                "SFXDevice error: stopMusic called without init!");

    _api->stopMusic();
}

void SFXDevice::stopAllSounds() {
    DIVIDE_ASSERT(_api != nullptr,
                "SFXDevice error: stopAllSounds called without init!");

    _api->stopAllSounds();
}

void SFXDevice::setMusicVolume(I8 value) {
    DIVIDE_ASSERT(_api != nullptr,
                "SFXDevice error: setMusicVolume called without init!");

    _api->setMusicVolume(value);
}

void SFXDevice::setSoundVolume(I8 value) {
    DIVIDE_ASSERT(_api != nullptr,
                "SFXDevice error: setSoundVolume called without init!");

    _api->setSoundVolume(value);
}

}; //namespace Divide
