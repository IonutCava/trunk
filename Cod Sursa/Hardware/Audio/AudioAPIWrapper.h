#ifndef _AUDIO_API_H
#define _AUDIO_API_H

#include "Hardware/Platform/PlatformDefines.h"
#include "AudioDescriptor.h"

class AudioState
{
public:
	AudioState(bool enableA,
		        bool enableB,
				bool enableC,
				bool enableD){}
};

enum AudioAPI
{
	FMOD,
	OpenAL,
	SDL
};


#include <iostream>
#include <unordered_map>

//Audio Programming Interface
class AudioAPIWrapper
{

protected:
	AudioAPIWrapper() : _state(AudioState(true,true,true,true)) {}

	friend class SFXDevice;
	
	void setId(AudioAPI api) {_apiId = api;}
	AudioAPI getId() { return _apiId;}

	
	virtual void initHardware() = 0;
	virtual void closeAudioApi() = 0;
	virtual void initDevice() = 0;

	virtual void playSound(AudioDescriptor* sound) = 0;
	virtual void playMusic(AudioDescriptor* music) = 0;

	virtual void pauseMusic() = 0;
	virtual void stopMusic() = 0;
	virtual void stopAllSounds() = 0;



	virtual void setMusicVolume(I8 value) = 0;
	virtual void setSoundVolume(I8 value) = 0;

	virtual ~AudioAPIWrapper(){};

public: //RenderAPI global
	
	void setAudioState(AudioState& state){_state = state; }
	AudioState& getActiveAudioState() {return _state;}

private:
	AudioAPI _apiId;

protected:
	AudioState _state;

};

#endif