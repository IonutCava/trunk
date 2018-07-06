#ifndef _PRE_RENDER_OPERATOR_H_
#define _PRE_RENDER_OPERATOR_H_

#include "Core/Math/Headers/MathMatrices.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

namespace Divide {

class Quad3D;
class Texture;
class ShaderProgram;
class SamplerDescriptor;

enum class RenderStage : U32;


// ALL FILTERS MUST MODIFY THE INPUT RENDERTARGET ONLY!
enum class FilterType : U32 {
    FILTER_SS_ANTIALIASING = 0,
    FILTER_SS_REFLECTIONS,
    FILTER_SS_AMBIENT_OCCLUSION,
    FILTER_DEPTH_OF_FIELD,
    FILTER_MOTION_BLUR,
    FILTER_BLOOM,
    FILTER_LUT_CORECTION,
    FILTER_UNDERWATER,
    FILTER_NOISE,
    FILTER_VIGNETTE,
    FILTER_COUNT
};

enum class FilterSpace : U32 {
    FILTER_SPACE_HDR = 0,
    FILTER_SPACE_LDR = 1,
    COUNT
};

typedef std::array<U8 /*request count*/, to_const_U32(FilterType::FILTER_COUNT)> FilterStack;

class PreRenderBatch;
/// It's called a prerender operator because it operates on the buffer before
/// "rendering" to the screen
/// Technically, it's a post render operation
class NOINITVTABLE PreRenderOperator {
   public:
    /// The RenderStage is used to inform the GFXDevice of what we are currently
    /// doing to set up apropriate states
    /// The target is the full screen quad to which we want to apply our
    /// operation to generate the result
    PreRenderOperator(GFXDevice& context, PreRenderBatch& parent, ResourceCache& cache, FilterType operatorType);
    virtual ~PreRenderOperator();

    virtual void idle(const Configuration& config) = 0;
    virtual void execute() = 0;

    virtual void reshape(U16 width, U16 height);

    inline FilterType operatorType() const { return _operatorType; }

    virtual void debugPreview(U8 slot) const;

    static void cacheDisplaySettings(const GFXDevice& context);

   protected:
    GFXDevice& _context;

    PreRenderBatch& _parent;

    RenderTargetHandle _samplerCopy;
    RTDrawDescriptor _screenOnlyDraw;
    FilterType  _operatorType;

    static F32 s_mainCamAspectRatio;
    static vec2<F32> s_mainCamZPlanes;
    static mat4<F32> s_mainCamViewMatrixCache;
    static mat4<F32> s_mainCamProjectionMatrixCache;
};

};  // namespace Divide
#endif
