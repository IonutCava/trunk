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
           _ldrTarget(ldrTarget),
           _samplerCopy(nullptr)
    {
        _screenOnlyDraw._drawMask.fill(false);
        _screenOnlyDraw._drawMask[0] = true;
    }

    virtual ~PreRenderOperator() 
    {
        MemoryManager::DELETE(_samplerCopy);
    }

    virtual void idle() = 0;
    virtual void execute() = 0;

    virtual void reshape(U16 width, U16 height) {
        if (_samplerCopy) {
            _samplerCopy->create(width, height);
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

   protected:
    RenderTarget* _hdrTarget;
    RenderTarget* _ldrTarget;
    RenderTarget* _samplerCopy;

    RenderTarget::RenderTargetDrawDescriptor _screenOnlyDraw;
    FilterType  _operatorType;
    vectorImpl<RenderTarget*> _inputFB;
};

};  // namespace Divide
#endif
