#include "SDLWrapper.h"
#include <stdexcept>

void SDL_API::initHardware()
{
	Mix_Init(MIX_INIT_OGG | MIX_INIT_MP3);
	_music = NULL;
	_chunk = NULL;
        
    if (-1 == Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 4096))
        throw std::runtime_error("Can't open audio");

}

void SDL_API::playMusic(AudioDescriptor* musicFile)
{
	if(!musicFile) return;
	_music = Mix_LoadMUS(musicFile->getName().c_str());
	Mix_VolumeMusic(musicFile->getVolume());
	 if (NULL != _music) {
		 if (-1 == Mix_PlayMusic(_music, musicFile->isLooping() ? -1 : 0))
            throw std::runtime_error("Can't play file");
    }
	 

}

void SDL_API::playSound(AudioDescriptor* sound)
{
	if(!sound) return;
	if(_chunk != NULL) Mix_FreeChunk(_chunk);
	_chunk  = Mix_LoadWAV(sound->getName().c_str());
	Mix_Volume(sound->getChannel(),sound->getVolume());

	if(_chunk == NULL)
		Con::getInstance().errorfn("SFXDevice: Can't load sound [ %s ] with SDL!" ,sound->getName().c_str());

	if(Mix_PlayChannel( sound->getChannel(), _chunk, sound->isLooping() ? -1 : 0 ) == -1){
		Con::getInstance().errorfn("SFXDevice: Can't play sound [ %s ] with SDL! Error: %s" ,sound->getName().c_str(),Mix_GetError());
	}
	
}