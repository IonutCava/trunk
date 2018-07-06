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

#ifndef _SFX_DEVICE_H
#define _SFX_DEVICE_H

#include "Hardware/Audio/fmod/Headers/FmodWrapper.h"
#include "Hardware/Audio/SDL_mixer/Headers/SDLWrapper.h"
#include "Hardware/Audio/openAl/Headers/ALWrapper.h"

namespace Divide {

DEFINE_SINGLETON_EXT1(SFXDevice,AudioAPIWrapper)

public:

	inline ErrorCode initAudioApi()  { return _api.initAudioApi(); }
	inline void      closeAudioApi() { _api.closeAudioApi(); }

	inline void playSound(AudioDescriptor* sound){_api.playSound(sound);}
	inline void playMusic(AudioDescriptor* music){_api.playMusic(music);}

	inline void pauseMusic()    {_api.pauseMusic();}
	inline void stopMusic()     {_api.stopMusic();}
	inline void stopAllSounds() {_api.stopAllSounds();}

	inline void setMusicVolume(I8 value){_api.setMusicVolume(value);}
	inline void setSoundVolume(I8 value){_api.setSoundVolume(value);}

private:
	SFXDevice(): _api(SDL_API::getOrCreateInstance()) //Defaulting to SDL if no api has been defined
	{
	}

	~SFXDevice()
	{
		SDL_API::destroyInstance();
	}

	AudioAPIWrapper& _api;

END_SINGLETON

#define SFX_DEVICE SFXDevice::getInstance()

}; //namespace Divide

#endif