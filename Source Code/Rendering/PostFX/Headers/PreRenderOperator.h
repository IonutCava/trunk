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
    FILTER_SS_ANTIALIASING = toBit(1),
    FILTER_SS_REFLECTIONS = toBit(2),
    FILTER_SS_AMBIENT_OCCLUSION = toBit(3),
    FILTER_DEPTH_OF_FIELD = toBit(4),
    FILTER_MOTION_BLUR = toBit(5),
    FILTER_BLOOM = toBit(6),
    FILTER_LUT_CORECTION = toBit(7)
};

enum class FilterSpace : U32 {
    FILTER_SPACE_HDR = 0,
    FILTER_SPACE_LDR = 1,
    COUNT
};

/// It's called a prerender operator because it operates on the buffer before
/// "rendering" to the screen
/// Technically, it's a post render operation
class NOINITVTABLE PreRenderOperator {
   public:
    /// The RenderStage is used to inform the GFXDevice of what we are currently
    /// doing to set up apropriate states
    /// The target is the full screen quad to which we want to apply our
    /// operation to generate the result
    PreRenderOperator(FilterType operatorType, RenderTarget* hdrTarget, RenderTarget* ldrTarget)
        :  _operatorType(operatorType),
           _hdrTarget(hdrTarget),
           _ldrTarget(ldrTarget)
    {
        _screenOnlyDraw.disableState(RTDrawDescriptor::State::CLEAR_DEPTH_BUFFER);
        _screenOnlyDraw.drawMask().disableAll();
        _screenOnlyDraw.drawMask().setEnabled(RTAttachment::Type::Colour, 0, true);
    }

    virtual ~PreRenderOperator() 
    {
        GFX_DEVICE.deallocateRT(_samplerCopy);
    }

    virtual void idle() = 0;
    virtual void execute() = 0;

    virtual void reshape(U16 width, U16 height) {
        if (_samplerCopy._rt) {
            _samplerCopy._rt->create(width, height);
        }
    }

    inline void addInputFB(RenderTarget* const input) {
        _inputFB.push_back(input);
    }

    inline FilterType operatorType() const {
        return _operatorType;
    }

    virtual void debugPreview(U8 slot) const {
    };

    virtual RenderTarget* getOutput() const {
        return _hdrTarget;
    }

    static void cacheDisplaySettings(const GFXDevice& context);

   protected:
    RenderTarget* _hdrTarget;
    RenderTarget* _ldrTarget;
    RenderTargetHandle _samplerCopy;

    RTDrawDescriptor _screenOnlyDraw;
    FilterType  _operatorType;
    vectorImpl<RenderTarget*> _inputFB;

    static F32 s_mainCamAspectRatio;
    static vec2<F32> s_mainCamZPlanes;
    static mat4<F32> s_mainCamViewMatrixCache;
    static mat4<F32> s_mainCamProjectionMatrixCache;
};

};  // namespace Divide
#endif
