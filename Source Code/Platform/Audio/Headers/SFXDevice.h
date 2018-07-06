/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _SFX_DEVICE_H
#define _SFX_DEVICE_H

#include "AudioAPIWrapper.h"

namespace Divide {

DEFINE_SINGLETON_EXT1_W_SPECIFIER(SFXDevice, AudioAPIWrapper, final)
  public:
    enum AudioAPI { FMOD, OpenAL, SDL, SFX_PLACEHOLDER };

    ErrorCode initAudioAPI();
    void closeAudioAPI();

    inline void setAPI(AudioAPI API) { _API_ID = API; }
    inline AudioAPI getAPI() const { return _API_ID; }

    inline void setAudioState(AudioState& state) { _state = state; }
    inline AudioState& getActiveAudioState() { return _state; }

    void playSound(AudioDescriptor* sound);
    void playMusic(AudioDescriptor* music);
    void pauseMusic();
    void stopMusic();
    void stopAllSounds();
    void setMusicVolume(I8 value);
    void setSoundVolume(I8 value);
    
  private:
    SFXDevice();
    ~SFXDevice();

  protected:
    AudioState _state;

  private:
    AudioAPI _API_ID;
    AudioAPIWrapper* _api;

END_SINGLETON

#define SFX_DEVICE SFXDevice::getInstance()

};  // namespace Divide

#endif