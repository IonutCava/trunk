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

#ifndef _WRAPPER_SDL_H_
#define _WRAPPER_SDL_H_

#define HAVE_M_PI
#include <SDL_mixer.h>
#include "Platform/Audio/Headers/AudioAPIWrapper.h"

namespace Divide {

DEFINE_SINGLETON_EXT1_W_SPECIFIER(SDL_API, AudioAPIWrapper, final)
  public:
    ErrorCode initAudioAPI();
    void closeAudioAPI();

    void playSound(AudioDescriptor* sound);
    void playMusic(AudioDescriptor* music);

    void stopMusic() { Mix_HaltMusic(); }
    void stopAllSounds() {}
    void pauseMusic() {}

    void setMusicVolume(I8 value) {}
    void setSoundVolume(I8 value) {}

  private:
    Mix_Music* _music;
    Mix_Chunk* _chunk;

END_SINGLETON

};  // namespace Divide

#endif
