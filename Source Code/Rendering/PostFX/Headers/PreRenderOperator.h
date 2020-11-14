#pragma once
#ifndef _PRE_RENDER_OPERATOR_H_
#define _PRE_RENDER_OPERATOR_H_

#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Textures/Headers/Texture.h"

namespace Divide {

class Quad3D;
class Texture;
class ShaderProgram;

enum class RenderStage : U8;

enum class FilterType : U16 {
    FILTER_SS_ANTIALIASING = toBit(1),
    FILTER_SS_AMBIENT_OCCLUSION = toBit(2),
    FILTER_SS_REFLECTIONS = toBit(3),
    FILTER_DEPTH_OF_FIELD = toBit(4),
    FILTER_MOTION_BLUR = toBit(5),
    FILTER_BLOOM = toBit(6),
    FILTER_LUT_CORECTION = toBit(7),
    FILTER_UNDERWATER = toBit(8),
    FILTER_NOISE = toBit(9),
    FILTER_VIGNETTE = toBit(10),
    FILTER_COUNT = 11
};

enum class FilterSpace : U8 {
    FILTER_SPACE_HDR = 0,
    FILTER_SPACE_LDR = 1,
    COUNT
};

class PreRenderBatch;
/// It's called a prerender operator because it operates on the buffer before "rendering" to the screen
/// Technically, it's a post render operation
class NOINITVTABLE PreRenderOperator {
   public:
    /// The RenderStage is used to inform the GFXDevice of what we are currently
    /// doing to set up appropriate states
    /// The target is the full screen quad to which we want to apply our
    /// operation to generate the result
    PreRenderOperator(GFXDevice& context, PreRenderBatch& parent, FilterType operatorType);
    virtual ~PreRenderOperator() = default;

    virtual void prepare(const Camera* camera, GFX::CommandBuffer& bufferInOut);
    /// Return true if we rendered into "output"
    virtual bool execute(const Camera* camera, const RenderTargetHandle& input, const RenderTargetHandle& output, GFX::CommandBuffer& bufferInOut);

    virtual void reshape(U16 width, U16 height);

    FilterType operatorType() const noexcept { return _operatorType; }

    virtual void idle(const Configuration& config);
    virtual void onToggle(bool state);

    virtual bool ready() const { return true; }

   protected:
    GFXDevice& _context;

    PreRenderBatch& _parent;
    GFX::DrawCommand _pointDrawCmd = {};
    GFX::DrawCommand _triangleDrawCmd = {};
    RTDrawDescriptor _screenOnlyDraw;
    FilterType  _operatorType = FilterType::FILTER_COUNT;
};

};  // namespace Divide
#endif
