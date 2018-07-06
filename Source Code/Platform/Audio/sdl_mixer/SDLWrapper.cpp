#include "Headers/SDLWrapper.h"
#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

#include <stdexcept>

namespace Divide {

ErrorCode SDL_API::initAudioAPI() {
    I32 flags = MIX_INIT_OGG | MIX_INIT_MP3;
    I32 ret = Mix_Init(flags);
    if ((ret & flags) == flags) {
        // Try HiFi sound
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS,
                          4096) == -1) {
            Console::errorfn("%s", Mix_GetError());
            // Try lower quality
            if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT,
                              MIX_DEFAULT_CHANNELS, 2048) == -1) {
                Console::errorfn("%s", Mix_GetError());
                return ErrorCode::SDL_AUDIO_MIX_INIT_ERROR;
            }
        }

        return ErrorCode::NO_ERR;
    }
    Console::errorfn("%s", Mix_GetError());
    return ErrorCode::SDL_AUDIO_INIT_ERROR;
}

void SDL_API::closeAudioAPI() {
    Mix_HaltMusic();
    for (MusicMap::value_type it : _musicMap) {
        Mix_FreeMusic(it.second);
    }
    for (SoundMap::value_type it : _soundMap) {
        Mix_FreeChunk(it.second);
    }
    Mix_CloseAudio();
    Mix_Quit();
}

void SDL_API::playMusic(const AudioDescriptor_ptr& music) {
    if (music) {
        Mix_Music* musicPtr = nullptr;
        MusicMap::iterator it = _musicMap.find(music->getGUID());
        if (it == std::cend(_musicMap)) {
            musicPtr = Mix_LoadMUS(music->getAudioFile().c_str());
            hashAlg::emplace(_musicMap, music->getGUID(), musicPtr);
        } else {
            if (music->dirty()) {
                musicPtr = Mix_LoadMUS(music->getAudioFile().c_str());
                Mix_FreeMusic(it->second);
                it->second = musicPtr;
                music->clean();
            } else {
                musicPtr = it->second;
            }
        }
        
        if(musicPtr) {
            Mix_VolumeMusic(music->getVolume());
            if (Mix_PlayMusic(musicPtr, music->isLooping() ? -1 : 0) == -1) {
                Console::errorfn("%s", Mix_GetError());
            }
        } else {
            Console::errorfn(Locale::get(_ID("ERROR_SDL_LOAD_SOUND")), music->getName().c_str());
        }
    }
}

void SDL_API::playSound(const AudioDescriptor_ptr& sound) {
    if (sound) {
        Mix_Chunk* soundPtr = nullptr;
        SoundMap::iterator it = _soundMap.find(sound->getGUID());
        if (it == std::cend(_soundMap)) {
            soundPtr = Mix_LoadWAV(sound->getAudioFile().c_str());
            hashAlg::emplace(_soundMap, sound->getGUID(), soundPtr);
        } else {
            if (sound->dirty()) {
                soundPtr = Mix_LoadWAV(sound->getAudioFile().c_str());
                Mix_FreeChunk(it->second);
                it->second = soundPtr;
                sound->clean();
            } else {
                soundPtr = it->second;
            }
        }

        if (soundPtr) {
            Mix_Volume(sound->getChannel(), sound->getVolume());
            if (Mix_PlayChannel(sound->getChannel(), soundPtr, sound->isLooping() ? -1 : 0) == -1) {
                Console::errorfn(Locale::get(_ID("ERROR_SDL_CANT_PLAY")), sound->getName().c_str(), Mix_GetError());
            }
        } else {
            Console::errorfn(Locale::get(_ID("ERROR_SDL_LOAD_SOUND")), sound->getName().c_str());
        }
    }   
}

};