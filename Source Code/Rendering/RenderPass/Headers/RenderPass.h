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
// Usefull to avoid having multiple renderqueues per pass if 2 stages depend on one:
// E.g.: DEPTH_PASS + DISPLAY share the same renderqueue
class RenderPass : private NonCopyable {
   public:
       struct BufferData {
           BufferData(GFXDevice& context, U32 sizeFactor, I32 index);
           ~BufferData();

           U32 _renderDataElementOffset = 0;
           ShaderBuffer* _renderData;

           vectorEASTL<ShaderBuffer*> _cmdBuffers;

           I32 _sizeFactor;
           U32 _lastCommandCount;
       };
   protected:

    struct BufferDataPoolParams {
        U32 _maxBuffers = 0;
        U32 _maxPassesPerBuffer = 0;
    };

    struct BufferDataPool {
        explicit BufferDataPool(GFXDevice& context, const BufferDataPoolParams& maxBuffers);
        ~BufferDataPool();

        BufferData& getBufferData(I32 bufferIndex, I32 bufferOffset);
        const BufferData& getBufferData(I32 bufferIndex, I32 bufferOffset) const;

        void initBuffers();

    private:
        GFXDevice& _context;
        U32 _bufferSizeFactor = 0;
        vector<std::shared_ptr<BufferData>> _buffers;
    };

   public:
    // passStageFlags: the first stage specified will determine the data format used by the additional stages in the list
    explicit RenderPass(RenderPassManager& parent, GFXDevice& context, stringImpl name, U8 sortKey, RenderStage passStageFlags);
    ~RenderPass();

    void render(const SceneRenderState& renderState, GFX::CommandBuffer& bufferInOut);
    inline U8 sortKey() const { return _sortKey; }
    inline U16 getLastTotalBinSize() const { return _lastTotalBinSize; }
    inline const stringImpl& name() const { return _name; }

    inline RenderStage stageFlag() const { return _stageFlag; }

    BufferData& getBufferData(I32 bufferIndex, I32 bufferOffset);
    const BufferData& getBufferData(I32 bufferIndex, I32 bufferOffset) const;

    void initBufferData();

   protected:
    
    BufferDataPoolParams getBufferParamsForStage(RenderStage stages) const;

   private:
    GFXDevice & _context;
    RenderPassManager& _parent;

    U8 _sortKey;
    stringImpl _name;
    U16 _lastTotalBinSize;
    RenderStage _stageFlag;
    BufferDataPool* _passBuffers;
};

};  // namespace Divide

#endif
