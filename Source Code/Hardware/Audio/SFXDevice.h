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

END_SINGLETON

#endif