/*
Copyright (c) 2017 DIVIDE-Studio
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

class RenderPackage {
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

    size_t drawCommandCount() const;
    const GenericDrawCommand& drawCommand(I32 index) const;
    void drawCommand(I32 index, const GenericDrawCommand cmd);

    const Pipeline& pipeline(I32 index) const;
    void pipeline(I32 index, const Pipeline& pipeline);

    const ClipPlaneList& clipPlanes(I32 index) const;
    void clipPlanes(I32 index, const ClipPlaneList& pipeline);

    const PushConstants& pushConstants(I32 index) const;
    void pushConstants(I32 index, const PushConstants& constants);

    const DescriptorSet& descriptorSet(I32 index) const;
    void descriptorSet(I32 index, const DescriptorSet& descriptorSets);

    void addDrawCommand(const GFX::DrawCommand& cmd);
    void addPipelineCommand(const GFX::BindPipelineCommand& clipPlanes);
    void addClipPlanesCommand(const GFX::SetClipPlanesCommand& clipPlanes);
    void addPushConstantsCommand(const GFX::SendPushConstantsCommand& pushConstants);
    void addDescriptorSetsCommand(const GFX::BindDescriptorSetsCommand& descriptorSets);

protected:
    friend class RenderingComponent;
    //eventually phase this out
    GFX::CommandBuffer& commands();
    GenericDrawCommand& drawCommand(I32 index);

private:
    GFXDevice& _context;
    bool _isRenderable;
    bool _isOcclusionCullable;
    bool _secondaryCommandPool;

    vectorImpl<GenericDrawCommand> _drawCommands;
    bool _drawCommandDirty = true;

    vectorImpl<Pipeline> _pipelines;
    bool _pipelineDirty = true;

    vectorImpl<ClipPlaneList> _clipPlanes;
    bool _clipPlanesDirty = true;

    vectorImpl<PushConstants> _pushConstants;
    bool _pushConstantsDirty = true;

    vectorImpl<DescriptorSet> _descriptorSets;
    bool _descriptorSetsDirty = true;

private:
    // Cached command buffer
    bool _commandBufferDirty = true;
    GFX::CommandBuffer* _commands;
};

}; //namespace Divide

#endif //_RENDER_PACKAGE_H_
