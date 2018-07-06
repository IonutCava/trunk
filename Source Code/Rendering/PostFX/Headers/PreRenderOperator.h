#ifndef _PRE_RENDER_OPERATOR_H_
#define _PRE_RENDER_OPERATOR_H_

#include "Core/Math/Headers/MathMatrices.h"

namespace Divide {

class Quad3D;
class Texture;
class Framebuffer;
class ShaderProgram;
class SamplerDescriptor;

enum class RenderStage : U32;

/// It's called a prerender operator because it operates on the buffer before
/// "rendering" to the screen
/// Technically, it's a post render operation
class NOINITVTABLE PreRenderOperator {
   protected:
    enum class PostFXRenderStage : U32 {
        FXAA = toBit(1),
        SSAO = toBit(2),
        DOF = toBit(3),
        BLOOM = toBit(4),
        LIGHT_SHAFT = toBit(5)
    };

   public:
    /// The RenderStage is used to inform the GFXDevice of what we are currently
    /// doing to set up apropriate states
    /// The target is the full screen quad to which we want to apply our
    /// operation to generate the result
    PreRenderOperator(PostFXRenderStage stage, const vec2<U16>& resolution,
                      SamplerDescriptor* const sampler)
        : _enabled(false),
          _genericFlag(false),
          _resolution(resolution),
          _internalSampler(sampler),
          _stage(stage)
    {
    }

    virtual ~PreRenderOperator() {}

    virtual void operation() = 0;
    virtual void reshape(U16 width, U16 height) = 0;
    /// Reference to state
    inline void enabled(bool state) { _enabled = state; }
    inline bool enabled() const { return _enabled; }
    inline void genericFlag(bool state) { _genericFlag = state; }
    inline bool genericFlag() const { return _genericFlag; }
    inline void addInputFB(Framebuffer* const input) {
        _inputFB.push_back(input);
    }

   protected:
    bool _enabled;
    /// Used to represent anything (for example: HDR on/off for bloom)
    bool _genericFlag;
    vectorImpl<Framebuffer*> _inputFB;
    vec2<U16> _resolution;
    SamplerDescriptor* _internalSampler;

   private:
    PostFXRenderStage _stage;
};

};  // namespace Divide
#endif
