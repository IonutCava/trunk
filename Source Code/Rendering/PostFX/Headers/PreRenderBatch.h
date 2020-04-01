#ifndef _PRE_RenderStage_H_
#define _PRE_RenderStage_H_

#include "PreRenderOperator.h"

namespace Divide {
struct ToneMapParams
{
    U16 width = 1u;
    U16 height = 1u;
    F32 minLogLuminance = -4.0f;
    F32 maxLogLuminance = 3.0f;
    F32 tau = 1.1f;

    F32 manualExposureAdaptive = 1.1f;
    F32 manualExposure = 0.975f;
    F32 manualWhitePoint = 0.975f;
};

class ResourceCache;
class PreRenderBatch {
   public:
    PreRenderBatch(GFXDevice& context, ResourceCache& cache);
    ~PreRenderBatch();

    void init(RenderTargetID renderTarget);
    void destroy();

    void idle(const Configuration& config);
    void update(const U64 deltaTimeUS);

    void prepare(const Camera& camera, U16 filterStack, GFX::CommandBuffer& bufferInOut);
    void execute(const Camera& camera, U16 filterStack, GFX::CommandBuffer& bufferInOut);
    void reshape(U16 width, U16 height);

    void onFilterEnabled(FilterType filter);
    void onFilterDisabled(FilterType filter);

    TextureData getOutput();

    RenderTargetHandle inputRT() const;
    RenderTargetHandle& outputRT();

    RenderTargetHandle luminanceRT() const;

    inline PreRenderOperator& getOperator(FilterType type) {
        const OperatorBatch& batch = _operators[to_U32(getOperatorSpace(type))];
        OperatorBatch::const_iterator it =
            std::find_if(std::cbegin(batch), std::cend(batch),
                [type](PreRenderOperator* op) {
            return op->operatorType() == type;
        });
        assert(it != std::cend(batch));
        return *(*it);
    }

    inline const PreRenderOperator& getOperator(FilterType type) const {
        const OperatorBatch& batch = _operators[to_U32(getOperatorSpace(type))];
        OperatorBatch::const_iterator it =
            std::find_if(std::cbegin(batch), std::cend(batch),
                [type](PreRenderOperator* op) {
            return op->operatorType() == type;
        });
        assert(it != std::cend(batch));
        return *(*it);
    }


    PROPERTY_RW(ToneMapParams, toneMapParams, {});
    PROPERTY_RW(bool,  adaptiveExposureControl, true);
   private:

    inline FilterSpace getOperatorSpace(FilterType type) const noexcept {
        switch(type) {
            case FilterType::FILTER_SS_ANTIALIASING :
                return FilterSpace::FILTER_SPACE_LDR;
        }

        return FilterSpace::FILTER_SPACE_HDR;
    }

    void onFilterToggle(FilterType type, const bool state);
  private:
    using OperatorBatch = vectorSTD<PreRenderOperator*>;
    OperatorBatch _operators[to_base(FilterSpace::COUNT)];

    GFXDevice& _context;
    ResourceCache& _resCache;

    RenderTargetID     _renderTarget = RenderTargetUsage::SCREEN;
    PreRenderOperator* _debugOperator = nullptr;
    ShaderBuffer*      _histogramBuffer = nullptr;
    RenderTargetHandle _postFXOutput;
    RenderTargetHandle _currentLuminance;
    ShaderProgram_ptr _toneMap;
    ShaderProgram_ptr _toneMapAdaptive;
    ShaderProgram_ptr _createHistogram;
    ShaderProgram_ptr _averageHistogram;
    PushConstants     _toneMapConstants;

    U64 _lastDeltaTimeUS = 0u;
};

};  // namespace Divide
#endif
