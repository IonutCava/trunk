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
#ifndef _RENDER_PACKAGE_H_
#define _RENDER_PACKAGE_H_

#include "CommandBuffer.h"

namespace Divide {

class RenderingComponent;
class RenderPassManager;

namespace Attorney {
    class RenderPackageRenderPassManager;
    class RenderPackageRenderingComponent;
};

class RenderPackage {
    friend class Attorney::RenderPackageRenderPassManager;
    friend class Attorney::RenderPackageRenderingComponent;
public:
    enum class MinQuality : U8 {
        FULL = 0,
        LOW,
        COUNT
    };

public:
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

public:
    explicit RenderPackage() noexcept;
    ~RenderPackage();

    void clear();
    void set(const RenderPackage& other);

    size_t getSortKeyHash() const;

    inline I32 drawCommandCount() const noexcept { return _drawCommandCount; }

    const GenericDrawCommand& drawCommand(I32 index, I32 cmdIndex) const;
    void drawCommand(I32 index, I32 cmdIndex, const GenericDrawCommand& cmd);

    const GFX::DrawCommand& drawCommand(I32 index) const;

    const Pipeline* pipeline(I32 index) const;
    void pipeline(I32 index, const Pipeline& pipeline);

    const FrustumClipPlanes& clipPlanes(I32 index) const;
    void clipPlanes(I32 index, const FrustumClipPlanes& pipeline);

    PushConstants& pushConstants(I32 index);
    const PushConstants& pushConstants(I32 index) const;
    void pushConstants(I32 index, const PushConstants& constants);

    DescriptorSet& descriptorSet(I32 index);
    const DescriptorSet& descriptorSet(I32 index) const;
    void descriptorSet(I32 index, const DescriptorSet& descriptorSets);

    void addDrawCommand(const GFX::DrawCommand& cmd);
    void addPipelineCommand(const GFX::BindPipelineCommand& clipPlanes);
    void addClipPlanesCommand(const GFX::SetClipPlanesCommand& clipPlanes);
    void addPushConstantsCommand(const GFX::SendPushConstantsCommand& pushConstants);
    void addDescriptorSetsCommand(const GFX::BindDescriptorSetsCommand& descriptorSets);
    void addCommandBuffer(const GFX::CommandBuffer& commandBuffer);

    void addShaderBuffer(I32 descriptorSetIndex, const ShaderBufferBinding& buffer);
    void setTexture(I32 descriptorSetIndex, const TextureData& data, U8 binding);

    void setDrawOption(CmdRenderOptions option, bool state);
    void enableOptions(U16 optionMask);
    void disableOptions(U16 optionMask);

    inline bool empty() const noexcept { return _commands == nullptr || _commands->empty(); }

    void setLoDIndexOffset(U8 lodIndex, size_t indexOffset, size_t indexCount) noexcept;

    PROPERTY_RW(bool, autoIndexBuffer,  false);
    PROPERTY_RW(bool, textureDataDirty, true);
    PROPERTY_RW(MinQuality, qualityRequirement, MinQuality::FULL);

protected:
    void updateDrawCommands(U32 dataIndex, U32 startOffset, U8 lodLevel);
    GFX::CommandBuffer* commands();

protected:
    // Cached command buffer
    GFX::CommandBuffer* _commands = nullptr;

private:
    std::array<std::pair<size_t, size_t>, 4> _lodIndexOffsets;
    I32 _drawCommandCount = 0;
    U16 _drawCommandOptions = to_U16(CmdRenderOptions::RENDER_GEOMETRY);
    bool _isInstanced = false;
};


namespace Attorney {
    class RenderPackageRenderPassManager {
        private:
        static GFX::CommandBuffer* getCommandBuffer(RenderPackage& pkg) {
            return pkg.commands();
        }

        friend class Divide::RenderPassManager;
    };

    class RenderPackageRenderingComponent {
        private:
        static void updateDrawCommands(RenderPackage& pkg, U32 dataIndex, U32 startOffset, U8 lodLevel) {
            pkg.updateDrawCommands(dataIndex, startOffset, lodLevel);
        }

        friend class Divide::RenderingComponent;
    };
}; // namespace Attorney
}; // namespace Divide

#endif //_RENDER_PACKAGE_H_
