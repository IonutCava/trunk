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

#ifndef _AUDIO_DESCRIPTOR_H_
#define _AUDIO_DESCRIPTOR_H_

#include "core.h"
#include "Core/Resources/Headers/Resource.h"

class AudioDescriptor : public Resource{

public:
	AudioDescriptor() :  Resource(),
						 _is3D(false), _volume(100),
						 _frequency(44.2f), _bitDepth(16),
						 _channelId(-1), _isLooping(false)
	{
	}

	~AudioDescriptor() {}

	bool  unload() {return true;}

	inline bool& isLooping()  {return _isLooping;}
	inline bool& is3D()       {return _is3D;}

	inline void  setVolume(I8 value) {_volume = value;}
	inline I32   getVolume()          {return _volume;}

	inline void  setFrequency(F32 value) {_frequency = value;}
	inline F32   getFrequency()          {return _frequency;}

	inline void setBitDepth(I8 bitDepth) {_bitDepth = bitDepth;}
	inline I8   getBitDepth()            {return _bitDepth;}
	inline void setChannel(I8 id)		 {_channelId = id;}
	inline I8   getChannel()			 {return _channelId;}

private:
	bool _isLooping, _is3D;
	F32 _frequency;
	I8  _bitDepth, _channelId,_volume;
};
#endif