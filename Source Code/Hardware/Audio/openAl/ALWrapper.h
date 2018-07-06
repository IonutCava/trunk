#ifndef _WRAPPER_AL_H_
#define _WRAPPER_AL_H_

#include "../AudioAPIWrapper.h"
#include "Utility/Headers/Singleton.h"

SINGLETON_BEGIN_EXT1(AL_API,AudioAPIWrapper)

public:
	void initHardware(){}
	void closeAudioApi(){}
	void initDevice(){}

	void playSound(AudioDescriptor* sound){}
	void playMusic(AudioDescriptor* music){}

	void pauseMusic(){}
	void stopMusic(){}
	void stopAllSounds(){}

	void setMusicVolume(I8 value){}
	void setSoundVolume(I8 value){}
SINGLETON_END()

#endif