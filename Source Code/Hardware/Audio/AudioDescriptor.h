#ifndef _AUDIO_DESCRIPTOR_H_
#define _AUDIO_DESCRIPTOR_H_
#include "resource.h"
#include "Utility/Headers/BaseClasses.h"

class AudioDescriptor : public Resource
{
public:
	AudioDescriptor() :  _is3D(false), _volume(100),
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

private:
	bool _isLooping, _is3D;
	F32 _frequency;
	I8  _bitDepth, _channelId,_volume;
};
#endif