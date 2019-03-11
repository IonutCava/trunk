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

#ifndef _RENDERING_RENDER_PASS_RENDERPASS_H_
#define _RENDERING_RENDER_PASS_RENDERPASS_H_

#include "Platform/Video/Headers/RenderAPIWrapper.h"

namespace Divide {

namespace Time {
    class ProfileTimer;
};

class SceneGraph;
class ShaderBuffer;
class SceneRenderState;
class RenderPassManager;
enum class RenderStage : U8;

// A RenderPass may contain multiple linked stages.
// Useful to avoid having multiple renderqueues per pass if 2 stages depend on one:
// E.g.: PRE_PASS + MAIN_PASS share the same renderqueue
class RenderPass : private NonCopyable {
   public:
       struct BufferData {
           U32 _renderDataElementOffset = 0;
           ShaderBuffer* _renderData = nullptr;

           U32* _lastCommandCount = nullptr;
           ShaderBuffer* _cmdBuffer = nullptr;
       };
  public:
    // passStageFlags: the first stage specified will determine the data format used by the additional stages in the list
    explicit RenderPass(RenderPassManager& parent, GFXDevice& context, stringImpl name, U8 sortKey, RenderStage passStageFlags, const vector<U8>& dependencies);
    ~RenderPass();

    void render(const SceneRenderState& renderState, GFX::CommandBuffer& bufferInOut);
    void postRender();

    inline U8 sortKey() const { return _sortKey; }
    inline const vector<U8>& dependencies() const { return _dependencies; }
    inline U16 getLastTotalBinSize() const { return _lastTotalBinSize; }
    inline const stringImpl& name() const { return _name; }

    inline RenderStage stageFlag() const { return _stageFlag; }

    BufferData getBufferData(RenderPassType type, I32 passIndex) const;

    void initBufferData();

   private:
    GFXDevice & _context;
    RenderPassManager& _parent;

    U8 _sortKey = 0;
    vector<U8> _dependencies;
    stringImpl _name = "";
    U16 _lastTotalBinSize = 0;
    RenderStage _stageFlag = RenderStage::COUNT;

    U32 _dataBufferSize = 0;
    ShaderBuffer* _renderData = nullptr;
    mutable vectorEASTL<std::pair<ShaderBuffer*, U32>> _cmdBuffers;
};

};  // namespace Divide

#endif
