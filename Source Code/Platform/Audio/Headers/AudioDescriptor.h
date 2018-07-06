/*
   Copyright (c) 2016 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _AUDIO_DESCRIPTOR_H_
#define _AUDIO_DESCRIPTOR_H_

#include "Core/Headers/Application.h"
#include "Core/Resources/Headers/Resource.h"

namespace Divide {

class AudioDescriptor : public Resource {
   public:
    AudioDescriptor(const stringImpl& name, const stringImpl& audioFileName, const stringImpl& audioFilePath)
        : Resource(ResourceType::DEFAULT, name, audioFileName, audioFilePath),
          _isLooping(false),
          _dirty(true),
          _is3D(false),
          _frequency(44.2f),
          _bitDepth(16),
          _channelID(-1),
          _volume(100)
   {
   }

    virtual ~AudioDescriptor()
    {

    }

    inline bool unload() { 
        return true;
    }

    inline const stringImpl& getAudioFile() const { 
        return getResourceLocation() + getResourceName();
    }

    inline void setAudioFile(const stringImpl& filePath) {
        FileWithPath ret = splitPathToNameAndLocation(filePath);
        setResourceName(ret._fileName);
        setResourceLocation(ret._path);
        _dirty = true;
    }


    inline bool& isLooping() { return _isLooping; }
    inline bool& is3D() { return _is3D; }

    inline void setVolume(I8 value) { _volume = value; }
    inline I32 getVolume() { return _volume; }

    inline void setFrequency(F32 value) { _frequency = value; }
    inline F32 getFrequency() { return _frequency; }

    inline void setBitDepth(I8 bitDepth) { _bitDepth = bitDepth; }
    inline I8 getBitDepth() { return _bitDepth; }

    inline void setChannel(I8 ID) { _channelID = ID; }
    inline I8 getChannel() { return _channelID; }

    inline bool dirty() const { return _dirty; }
    inline void clean() { _dirty = false; }
   private:
    bool _dirty;
    bool _isLooping, _is3D;
    F32 _frequency;
    I8 _bitDepth, _channelID, _volume;
};

TYPEDEF_SMART_POINTERS_FOR_CLASS(AudioDescriptor);

};  // namespace Divide

#endif
