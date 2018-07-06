#include "Headers/SDLWrapper.h"
#include <stdexcept>

ErrorCodes SDL_API::initAudioApi() {
    Mix_Init(MIX_INIT_OGG | MIX_INIT_MP3);
    _music = nullptr;
    _chunk = nullptr;

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096) == -1) {
        return SDL_AUDIO_INIT_ERROR;
    }

    return NO_ERR;
}

void SDL_API::closeAudioApi() {
	if (_music != nullptr) {
		Mix_HaltMusic();
		Mix_FreeMusic(_music);
	}

	if(_chunk != nullptr) {
		Mix_FreeChunk(_chunk);
    }

	Mix_Quit();
}

void SDL_API::playMusic(AudioDescriptor* musicFile){
    if(!musicFile) return;
    _music = Mix_LoadMUS(musicFile->getAudioFile().c_str());
    Mix_VolumeMusic(musicFile->getVolume());
     if (nullptr != _music) {
         if (-1 == Mix_PlayMusic(_music, musicFile->isLooping() ? -1 : 0))
            throw std::runtime_error("Can't play file");
    }
}

void SDL_API::playSound(AudioDescriptor* sound){
    if(sound == nullptr) return;

    if(_chunk != nullptr) Mix_FreeChunk(_chunk);
    _chunk  = Mix_LoadWAV(sound->getAudioFile().c_str());
    assert(_chunk);
    Mix_Volume(sound->getChannel(),sound->getVolume());

    if(_chunk == nullptr)
        ERROR_FN(Locale::get("ERROR_SDL_LOAD_SOUND") ,sound->getName().c_str());

    if(Mix_PlayChannel( sound->getChannel(), _chunk, sound->isLooping() ? -1 : 0 ) == -1){
        ERROR_FN(Locale::get("ERROR_SDL_CANT_PLAY") ,sound->getName().c_str(),Mix_GetError());
    }
}