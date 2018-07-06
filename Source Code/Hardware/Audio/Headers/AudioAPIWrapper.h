/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

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

///Audio Programming Interface
class AudioAPIWrapper
{
protected:
	AudioAPIWrapper() : _apiId(SDL), _state(AudioState(true,true,true,true)) {}

	friend class SFXDevice;

	inline void setId(AudioAPI api) {_apiId = api;}
	inline AudioAPI getId() { return _apiId;}

	virtual I8   initAudioApi() = 0;
	virtual void closeAudioApi() = 0;

	virtual void playSound(AudioDescriptor* sound) = 0;
	virtual void playMusic(AudioDescriptor* music) = 0;

	virtual void pauseMusic() = 0;
	virtual void stopMusic() = 0;
	virtual void stopAllSounds() = 0;

	virtual void setMusicVolume(I8 value) = 0;
	virtual void setSoundVolume(I8 value) = 0;

	virtual ~AudioAPIWrapper(){};

public: //RenderAPI global

	inline void setAudioState(AudioState& state){_state = state; }
	inline AudioState& getActiveAudioState() {return _state;}

private:
	AudioAPI _apiId;

protected:
	AudioState _state;
};

#endif