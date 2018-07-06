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

#include "resource.h"
#include "Core/Headers/BaseClasses.h"

class AudioDescriptor : public Resource{

public:
	AudioDescriptor() :  Resource(),
						 _is3D(false), _volume(100),
						 _frequency(44.2f), _bitDepth(16),
						 _channelId(-1), _isLooping(false)
	{
	}

	~AudioDescriptor() {}

	bool  load(const std::string& name) {_name = name; return true;}
	bool  unload() {return true;}

	bool& isLooping()  {return _isLooping;}
	bool& is3D()       {return _is3D;}

	void  setVolume(I8 value) {_volume = value;}
	I32   getVolume()          {return _volume;}

	void  setFrequency(F32 value) {_frequency = value;}
	F32   getFrequency()          {return _frequency;}

	void  setBitDepth(I8 bitDepth) {_bitDepth = bitDepth;}
	I8    getBitDepth()            {return _bitDepth;}

	void setChannel(I8 id)		   {_channelId = id;}
	I8   getChannel()			   {return _channelId;}
	void createCopy(){incRefCount();}
	void removeCopy(){decRefCount();}
private:
	bool _isLooping, _is3D;
	F32 _frequency;
	I8  _bitDepth, _channelId,_volume;
};
#endif