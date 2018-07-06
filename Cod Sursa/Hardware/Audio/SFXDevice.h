#ifndef _SFX_DEVICE_H
#define _SFX_DEVICE_H

#include "fmod/FmodWrapper.h"
#include "SDL_mixer/SDLWrapper.h"
#include "openAl/ALWrapper.h"

SINGLETON_BEGIN_EXT1(SFXDevice,AudioAPIWrapper)

public:
	
	void initHardware(){_api.initHardware();}
	void closeAudioApi(){_api.closeAudioApi();}
	void initDevice(){_api.initDevice();}

	void playSound(AudioDescriptor* sound){_api.playSound(sound);}
	void playMusic(AudioDescriptor* music){_api.playMusic(music);}

	void pauseMusic(){_api.pauseMusic();}
	void stopMusic(){_api.stopMusic();}
	void stopAllSounds(){_api.stopAllSounds();}

	void setMusicVolume(I8 value){_api.setMusicVolume(value);}
	void setSoundVolume(I8 value){_api.setSoundVolume(value);}

private:
	SFXDevice(): _api(SDL_API::getInstance()) //Defaulting to SDL if no api has been defined
	{
	}

	AudioAPIWrapper& _api;

SINGLETON_END()

#endif