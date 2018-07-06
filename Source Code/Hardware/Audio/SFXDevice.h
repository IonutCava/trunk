/*“Copyright 2009-2012 DIVIDE-Studio”*/
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

#ifndef _SFX_DEVICE_H
#define _SFX_DEVICE_H

#include "fmod/FmodWrapper.h"
#include "SDL_mixer/SDLWrapper.h"
#include "openAl/ALWrapper.h"

DEFINE_SINGLETON_EXT1(SFXDevice,AudioAPIWrapper)

public:
	
	inline I8 initHardware()    {return _api.initHardware();}
	inline void closeAudioApi() {_api.closeAudioApi();}
	inline void initDevice()    {_api.initDevice();}

	inline void playSound(AudioDescriptor* sound){_api.playSound(sound);}
	inline void playMusic(AudioDescriptor* music){_api.playMusic(music);}

	inline void pauseMusic()    {_api.pauseMusic();}
	inline void stopMusic()     {_api.stopMusic();}
	inline void stopAllSounds() {_api.stopAllSounds();}

	inline void setMusicVolume(I8 value){_api.setMusicVolume(value);}
	inline void setSoundVolume(I8 value){_api.setSoundVolume(value);}

private:
	SFXDevice(): _api(SDL_API::getInstance()) //Defaulting to SDL if no api has been defined
	{
	}

	AudioAPIWrapper& _api;

END_SINGLETON

#define SFX_DEVICE SFXDevice::getInstance()
#endif