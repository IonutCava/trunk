/*�Copyright 2009-2012 DIVIDE-Studio�*/
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

#ifndef _WRAPPER_AL_H_
#define _WRAPPER_AL_H_

#include "../AudioAPIWrapper.h"

DEFINE_SINGLETON_EXT1(AL_API,AudioAPIWrapper)

public:
	I8 initHardware(){return OAL_INIT_ERROR;}
	void closeAudioApi(){}
	void initDevice(){}

	void playSound(AudioDescriptor* sound){}
	void playMusic(AudioDescriptor* music){}

	void pauseMusic(){}
	void stopMusic(){}
	void stopAllSounds(){}

	void setMusicVolume(I8 value){}
	void setSoundVolume(I8 value){}
END_SINGLETON

#endif