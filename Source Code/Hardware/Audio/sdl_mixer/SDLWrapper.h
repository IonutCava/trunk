#ifndef _WRAPPER_SDL_H_
#define _WRAPPER_SDL_H_

#include "SDL_mixer.h"
#include "../AudioAPIWrapper.h"
#include "Utility/Headers/Singleton.h"

SINGLETON_BEGIN_EXT1(SDL_API,AudioAPIWrapper)

public:
	void initHardware();

	void closeAudioApi(){
		if(_music)
			Mix_FreeMusic(_music);
		if(_chunk)
			Mix_FreeChunk(_chunk);
		Mix_Quit();
	}

	void initDevice(){}

	void playSound(AudioDescriptor* sound);
	void playMusic(AudioDescriptor* music);

	void stopMusic(){Mix_HaltMusic();}
	void stopAllSounds(){}
	void pauseMusic(){}

	void setMusicVolume(I8 value){}
	void setSoundVolume(I8 value){}

private:
	Mix_Music *_music;
	Mix_Chunk *_chunk;

SINGLETON_END()

#endif