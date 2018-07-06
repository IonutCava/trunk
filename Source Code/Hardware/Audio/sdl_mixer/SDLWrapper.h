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

#ifndef _WRAPPER_SDL_H_
#define _WRAPPER_SDL_H_

#include "SDL_mixer.h"
#include "../AudioAPIWrapper.h"

DEFINE_SINGLETON_EXT1(SDL_API,AudioAPIWrapper)

public:
	void initHardware();

	void closeAudioApi(){
		if(_music != NULL)
			Mix_FreeMusic(_music);
		if(_chunk != NULL)
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

END_SINGLETON

#endif