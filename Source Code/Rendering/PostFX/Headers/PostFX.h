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

#ifndef _POST_EFFECTS_H
#define _POST_EFFECTS_H

#include "PreRenderBatch.h"
#include "Core/Math/Headers/MathVectors.h"

namespace Divide {

class Quad3D;
class Camera;
class GFXDevice;
class ShaderProgram;
class Texture;

DEFINE_SINGLETON(PostFX)
   private:
      enum class TexOperatorBindPoint : U32 {
          TEX_BIND_POINT_SCREEN = 0,
          TEX_BIND_POINT_BORDER = 1,
          TEX_BIND_POINT_NOISE = 2,
          TEX_BIND_POINT_UNDERWATER = 3,
          COUNT
      };

  private:
    ~PostFX();
    PostFX();

  public:
    void apply();

    void init();
    void idle();
    void update(const U64 deltaTime);

    void updateResolution(U16 newWidth, U16 newHeight);

    inline void pushFilter(FilterType filter) {
        ++_filterStackCount[to_uint(filter)];
    }

    inline void popFilter(FilterType filter) {
        // Since this is scriptable, we may pop without a previous push
        // just to make sure we are in a proper state
        if (_filterStackCount[to_uint(filter)] > 0) {
            --_filterStackCount[to_uint(filter)];
        }
    }

    inline bool getFilterState(FilterType filter) const {
        return _filterStackCount[to_uint(filter)] > 0;
    }

    // fade the screen to the specified colour lerping over the specified time interval
    // set durationMS to instantly set fade colour
    // optionally, set a callback when fade out completes
    // waitDurationMS = how much time to wait vefore calling the completeion callback after fade out completes
    void setFadeOut(const vec4<U8>& targetColour, D64 durationMS, D64 waitDurationMS, DELEGATE_CBK<> onComplete = DELEGATE_CBK<>());
    // clear any fading effect currently active over the specified time interval
    // set durationMS to instantly clear the fade effect
    // optionally, set a callback when fade in completes
    void setFadeIn(D64 durationMS, DELEGATE_CBK<> onComplete = DELEGATE_CBK<>());
    // fade out to specified colour and back again within the given time slice
    // if duration is 0.0, nothing happens
    // waitDurationMS is the ammount of time to wait before fading back in
    void setFadeOutIn(const vec4<U8>& targetColour, D64 durationMS, D64 waitDurationMS);
    void setFadeOutIn(const vec4<U8>& targetColour, D64 durationFadeOutMS, D64 durationFadeInMS, D64 waitDurationMS);

    inline void cacheDisplaySettings(const GFXDevice& context) {
        PreRenderOperator::cacheDisplaySettings(context);
    }
  private:

    PreRenderBatch _preRenderBatch;
    /// Screen Border
    Texture_ptr _screenBorder;
    /// Noise
    Texture_ptr _noise;

    F32 _randomNoiseCoefficient, _randomFlashCoefficient;
    D64 _noiseTimer, _tickInterval;

    ShaderProgram_ptr _postProcessingShader;
    Texture_ptr _underwaterTexture;
    GFXDevice* _gfx;
    vec2<U16> _resolutionCache;
    vectorImpl<U32> _shaderFunctionSelection;
    vectorImpl<I32> _shaderFunctionList;

    RTDrawDescriptor _postFXTarget;

    //fade settings
    D64 _currentFadeTimeMS;
    D64 _targetFadeTimeMS;
    D64 _fadeWaitDurationMS;
    bool _fadeOut;
    bool _fadeActive;
    DELEGATE_CBK<> _fadeOutComplete;
    DELEGATE_CBK<> _fadeInComplete;

    FilterStack _filterStackCount;
END_SINGLETON

};  // namespace Divide

#endif
