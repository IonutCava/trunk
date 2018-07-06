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

#ifndef _RENDERING_RENDER_PASS_RENDERPASS_H_
#define _RENDERING_RENDER_PASS_RENDERPASS_H_

#include "Platform/Headers/PlatformDefines.h"
namespace Divide {

class SceneGraph;
class ShaderBuffer;
class SceneRenderState;
enum class RenderStage : U32;

// A RenderPass may contain multiple linked stages.
// Usefull to avoid having multiple renderqueues per pass if 2 stages depend on one:
// E.g.: Z_PRE_PASS + DISPLAY share the same renderqueue
class RenderPass : private NonCopyable {
   public:
       struct BufferData {
           BufferData();
           ~BufferData();

           ShaderBuffer* _renderData;
           ShaderBuffer* _cmdBuffer;
           U32 _lastCommandCount;
           U32 _lasNodeCount;
       };
   protected:

    struct BufferDataPool {
        explicit BufferDataPool(U32 maxPasses, U32 maxStages);
        ~BufferDataPool();

        BufferData& getBufferData(U32 pass, I32 idx);
        typedef vectorImpl<BufferData*> BufferPool;
        vectorImpl<BufferPool> _buffers;
    };

   public:
    // passStageFlags: the first stage specified will determine the data format used by the additional stages in the list
    RenderPass(stringImpl name, U8 sortKey, std::initializer_list<RenderStage> passStageFlags);
    ~RenderPass();

    void render(SceneRenderState& renderState);
    inline U8 sortKey() const { return _sortKey; }
    inline U16 getLastTotalBinSize() const { return _lastTotalBinSize; }
    inline const stringImpl& getName() const { return _name; }

    inline bool hasStageFlag(RenderStage stageFlag) const {
        return std::find_if(std::cbegin(_stageFlags), std::cend(_stageFlags),
                            [&stageFlag](RenderStage stage) {
                                return (stage == stageFlag);
                            }) != std::cend(_stageFlags);
    }


    BufferData& getBufferData(U32 pass, I32 idx);
   protected:
    bool preRender(SceneRenderState& renderState, U32 pass);
    bool postRender(SceneRenderState& renderState, U32 pass);

    std::pair<U32, U32> getRenderPassInfoForStages(const vectorImpl<RenderStage>& stages) const;
   private:
    U8 _sortKey;
    stringImpl _name;
    bool _useZPrePass;
    U16 _lastTotalBinSize;
    vectorImpl<RenderStage> _stageFlags;
    BufferDataPool* _passBuffers;
};

};  // namespace Divide

#endif
