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

#ifndef _RENDERING_RENDER_PASS_RENDERPASS_H_
#define _RENDERING_RENDER_PASS_RENDERPASS_H_

#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

class SceneGraph;
class SceneRenderState;
enum class RenderStage : U32;

// A RenderPass may contain multiple linked stages.
// Usefull to avoid having multiple renderqueues per pass if 2 stages depend on one:
// E.g.: Z_PRE_PASS + DISPLAY share the same renderqueue
class RenderPass {
   public:
    RenderPass(stringImpl name, U8 sortKey, std::initializer_list<RenderStage> passStageFlags);
    ~RenderPass();

    void render(SceneRenderState& renderState, bool anaglyph = false);
    inline U8 sortKey() const { return _sortKey; }
    inline U16 getLastTotalBinSize() const { return _lastTotalBinSize; }
    inline const stringImpl& getName() const { return _name; }

    inline bool hasStageFlag(RenderStage stageFlag) const {
        return std::find_if(std::cbegin(_stageFlags), std::cend(_stageFlags),
                            [&stageFlag](RenderStage stage) {
                                return (stage == stageFlag);
                            }) != std::cend(_stageFlags);
    }

    inline bool specialFlag() const { return _specialFlag; }
    inline void specialFlag(const bool state) { _specialFlag = state; }

   protected:
    bool preRender(SceneRenderState& renderState, bool anaglyph, U32 pass);
    bool postRender(SceneRenderState& renderState, bool anaglyph, U32 pass);

   private:
    U8 _sortKey;
    /// Used if some renderpasses share the same passStageFlag.
    /// Usefull to differentiate passes
    bool _specialFlag;
    stringImpl _name;
    U16 _lastTotalBinSize;
    vectorImpl<RenderStage> _stageFlags;
};

};  // namespace Divide

#endif
