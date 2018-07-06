/*“Copyright 2009-2011 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _AUDIO_API_H
#define _AUDIO_API_H

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


//Audio Programming Interface
class AudioAPIWrapper
{

protected:
	AudioAPIWrapper() : _apiId(SDL), _state(AudioState(true,true,true,true)) {}

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