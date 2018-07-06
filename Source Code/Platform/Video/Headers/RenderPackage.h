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

#ifndef _RENDER_PACKAGE_H_
#define _RENDER_PACKAGE_H_

#include "CommandBuffer.h"

namespace Divide {

class RenderPackageQueue;
class RenderingComponent;
namespace Attorney {
    class RenderPackageRenderingComponent;
    class RenderPackageRenderPackageQueue;
};

class RenderPackage {
    friend class Attorney::RenderPackageRenderingComponent;
    friend class Attorney::RenderPackageRenderPackageQueue;

public:
private:
    enum class CommandType : U8 {
        NONE = 0,
        DRAW = toBit(1),
        PIPELINE = toBit(2),
        CLIP_PLANES = toBit(3),
        PUSH_CONSTANTS = toBit(4),
        DESCRIPTOR_SETS = toBit(5),
        ALL = DRAW | PIPELINE | CLIP_PLANES | PUSH_CONSTANTS | DESCRIPTOR_SETS,
        COUNT = 7
    };

    struct CommandEntry {
        size_t _index = 0;
        GFX::CommandType _type = GFX::CommandType::COUNT;
    };

public:
    explicit RenderPackage(GFXDevice& context, bool useSecondaryBuffers);
    ~RenderPackage();

    void clear();
    void set(const RenderPackage& other);

    inline void isRenderable(bool state) { _isRenderable = state; }
    inline bool isRenderable() const { return  _isRenderable; }

    inline void isOcclusionCullable(bool state) { _isOcclusionCullable = state; }
    inline bool isOcclusionCullable() const { return  _isOcclusionCullable; }

    size_t getSortKeyHash() const;

    const GFX::CommandBuffer& commands() const;

    I32 drawCommandCount() const;
    const GenericDrawCommand& drawCommand(I32 index, I32 cmdIndex) const;
    void drawCommand(I32 index, I32 cmdIndex, const GenericDrawCommand& cmd);

    const GFX::DrawCommand& drawCommand(I32 index) const;

    const Pipeline* pipeline(I32 index) const;
    void pipeline(I32 index, const Pipeline& pipeline);

    const FrustumClipPlanes& clipPlanes(I32 index) const;
    void clipPlanes(I32 index, const FrustumClipPlanes& pipeline);

    const PushConstants& pushConstants(I32 index) const;
    void pushConstants(I32 index, const PushConstants& constants);

    const DescriptorSet& descriptorSet(I32 index) const;
    void descriptorSet(I32 index, const DescriptorSet& descriptorSets);

    void addDrawCommand(const GFX::DrawCommand& cmd);
    void addPipelineCommand(const GFX::BindPipelineCommand& clipPlanes);
    void addClipPlanesCommand(const GFX::SetClipPlanesCommand& clipPlanes);
    void addPushConstantsCommand(const GFX::SendPushConstantsCommand& pushConstants);
    void addDescriptorSetsCommand(const GFX::BindDescriptorSetsCommand& descriptorSets);
    void addCommandBuffer(const GFX::CommandBuffer& commandBuffer);
 
protected:
    // Return true if the command buffer was reconstructed
    bool buildCommandBuffer();
    GFX::DrawCommand& drawCommand(I32 cmdIdx);

private:
    GFXDevice& _context;
    bool _isRenderable;
    bool _isOcclusionCullable;
    bool _secondaryCommandPool;

    vectorImpl<GFX::DrawCommand> _drawCommands;
    vectorImpl<GFX::BindPipelineCommand> _pipelines;
    vectorImpl<GFX::SetClipPlanesCommand> _clipPlanes;
    vectorImpl<GFX::SendPushConstantsCommand> _pushConstants;
    vectorImpl<GFX::BindDescriptorSetsCommand> _descriptorSets;

protected:
    U32 _dirtyFlags = 0;
    vectorImpl<CommandEntry> _commandOrdering;
    // Cached command buffer
    bool _commandBufferDirty = true;
    GFX::CommandBuffer* _commands;
};

namespace Attorney {
    class RenderPackageRenderingComponent {
        private:
        static GFX::CommandBuffer* commands(const RenderPackage& pkg) {
            return pkg._commands;
        }

        // Return true if the command buffer was reconstructed
        static bool buildCommandBuffer(RenderPackage& pkg) {
            return pkg.buildCommandBuffer();
        }

        static GFX::DrawCommand& drawCommand(RenderPackage& pkg, I32 cmdIdx) {
            return pkg.drawCommand(cmdIdx);
        }

        friend class Divide::RenderingComponent;
    };

    class RenderPackageRenderPackageQueue {
        private:
        static GFX::CommandBuffer* commands(const RenderPackage& pkg) {
            return pkg._commands;
        }

        // Return true if the command buffer was reconstructed
        static bool buildCommandBuffer(RenderPackage& pkg) {
            return pkg.buildCommandBuffer();
        }

        friend class Divide::RenderPackageQueue;
    };
}; // namespace Attorney
}; // namespace Divide

#endif //_RENDER_PACKAGE_H_
