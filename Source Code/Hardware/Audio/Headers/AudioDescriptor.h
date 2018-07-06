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

#ifndef _AUDIO_DESCRIPTOR_H_
#define _AUDIO_DESCRIPTOR_H_

#include "core.h"
#include "Core/Resources/Headers/Resource.h"

namespace Divide {

class AudioDescriptor : public Resource{
public:
	AudioDescriptor(const stringImpl& audioFile) : Resource(audioFile), 
                         _audioFile(audioFile),
						 _is3D(false), _volume(100),
						 _frequency(44.2f), _bitDepth(16),
						 _channelId(-1), _isLooping(false)
	{
	}

	~AudioDescriptor() {}

	bool  unload() {return true;}
	inline const stringImpl& getAudioFile() {return _audioFile;}
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
	stringImpl _audioFile;
};

}; //namespace Divide

#endif