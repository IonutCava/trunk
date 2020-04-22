/*
   Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#pragma once
#ifndef _AUDIO_DESCRIPTOR_H_
#define _AUDIO_DESCRIPTOR_H_

#include "Core/Headers/Application.h"
#include "Core/Resources/Headers/Resource.h"
#include "Platform/File/Headers/FileManagement.h"

namespace Divide {

class AudioDescriptor : public CachedResource {
   public:
    AudioDescriptor(size_t descriptorHash, const Str128& name, const Str128& audioFileName, const stringImpl& audioFilePath)
        : CachedResource(ResourceType::DEFAULT, descriptorHash, name, audioFileName, audioFilePath),
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

    inline bool unload() override {
        return true;
    }

    inline void setAudioFile(const stringImpl& filePath) {
        FileWithPath ret = splitPathToNameAndLocation(filePath.c_str());
        assetName(ret._fileName);
        assetLocation(ret._path);
        _dirty = true;
    }


    inline bool& isLooping() noexcept { return _isLooping; }
    inline bool& is3D() noexcept { return _is3D; }

    inline void setVolume(I8 value) noexcept { _volume = value; }
    inline I32 getVolume() noexcept { return _volume; }

    inline void setFrequency(F32 value) noexcept { _frequency = value; }
    inline F32 getFrequency() noexcept { return _frequency; }

    inline void setBitDepth(I8 bitDepth) noexcept { _bitDepth = bitDepth; }
    inline I8 getBitDepth() noexcept { return _bitDepth; }

    inline void setChannel(I8 ID) noexcept { _channelID = ID; }
    inline I8 getChannel() noexcept { return _channelID; }

    inline bool dirty() const noexcept { return _dirty; }
    inline void clean() noexcept { _dirty = false; }

    const char* getResourceTypeName() const noexcept  override { return "AudioDescriptor"; }

   private:
    bool _dirty;
    bool _isLooping, _is3D;
    F32 _frequency;
    I8 _bitDepth, _channelID, _volume;
};

TYPEDEF_SMART_POINTERS_FOR_TYPE(AudioDescriptor);

};  // namespace Divide

#endif
