#ifndef _PRE_RenderStage_H_
#define _PRE_RenderStage_H_

#include "PreRenderOperator.h"

namespace Divide {

class PostFX;

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
       // Ordered by cost
       enum class EdgeDetectionMethod : U8 {
           Depth = 0,
           Luma,
           Colour,
           COUNT
       };
   public:
    PreRenderBatch(GFXDevice& context, PostFX& parent, ResourceCache& cache, RenderTargetID renderTarget);
    ~PreRenderBatch();

    PostFX& parent() const noexcept { return _parent; }

    void idle(const Configuration& config);
    void update(const U64 deltaTimeUS);

    void prepare(const Camera& camera, U16 filterStack, GFX::CommandBuffer& bufferInOut);
    void execute(const Camera& camera, U16 filterStack, GFX::CommandBuffer& bufferInOut);
    void reshape(U16 width, U16 height);

    void onFilterEnabled(FilterType filter);
    void onFilterDisabled(FilterType filter);

    TextureData getOutput();

    RenderTargetHandle& outputRT() noexcept;

    RenderTargetHandle inputRT() const noexcept;
    RenderTargetHandle edgesRT() const noexcept;
    RenderTargetHandle luminanceRT() const noexcept;

    inline PreRenderOperator* getOperator(FilterType type) {
        FilterSpace fSpace = getOperatorSpace(type);
        if (fSpace == FilterSpace::COUNT) {
            return nullptr;
        }

        const OperatorBatch& batch = _operators[to_U32(fSpace)];
        auto it = std::find_if(std::cbegin(batch), std::cend(batch), [type](PreRenderOperator* op) {
                                    return op->operatorType() == type;
                              });

        assert(it != std::cend(batch));
        return (*it);
    }

    inline const PreRenderOperator* getOperator(FilterType type) const {
        FilterSpace fSpace = getOperatorSpace(type);
        if (fSpace == FilterSpace::COUNT) {
            return nullptr;
        }

        const OperatorBatch& batch = _operators[to_U32(getOperatorSpace(type))];
        auto it = std::find_if(std::cbegin(batch), std::cend(batch), [type](PreRenderOperator* op) {
                                    return op->operatorType() == type;
                              });
        assert(it != std::cend(batch));
        return (*it);
    }


    PROPERTY_RW(ToneMapParams, toneMapParams, {});
    PROPERTY_RW(F32, edgeDetectionThreshold, 0.1f);
    PROPERTY_RW(bool,  adaptiveExposureControl, true);
    PROPERTY_RW(EdgeDetectionMethod, edgeDetectionMethod, EdgeDetectionMethod::Luma);

   private:

    inline FilterSpace getOperatorSpace(FilterType type) const noexcept {
        switch(type) {
            case FilterType::FILTER_SS_ANTIALIASING :
                return FilterSpace::FILTER_SPACE_LDR;
        }
        switch (type) {
            case FilterType::FILTER_NOISE:
            case FilterType::FILTER_VIGNETTE:
            case FilterType::FILTER_UNDERWATER:
                return FilterSpace::COUNT;
        }
        return FilterSpace::FILTER_SPACE_HDR;
    }

    void onFilterToggle(FilterType type, const bool state);

    void init();
    void destroy();
  private:
    using OperatorBatch = vectorSTD<PreRenderOperator*>;
    std::array<OperatorBatch, to_base(FilterSpace::COUNT)> _operators;

    GFXDevice& _context;
    PostFX&    _parent;
    ResourceCache& _resCache;

    RenderTargetID     _renderTarget = RenderTargetUsage::SCREEN;
    PreRenderOperator* _debugOperator = nullptr;
    ShaderBuffer*      _histogramBuffer = nullptr;
    RenderTargetHandle _postFXOutput;
    RenderTargetHandle _sceneEdges;
    RenderTargetHandle _currentLuminance;
    ShaderProgram_ptr _toneMap;
    ShaderProgram_ptr _toneMapAdaptive;
    ShaderProgram_ptr _createHistogram;
    ShaderProgram_ptr _averageHistogram;
    std::array<ShaderProgram_ptr, to_base(EdgeDetectionMethod::COUNT)> _edgeDetection;
    std::array<Pipeline*, to_base(EdgeDetectionMethod::COUNT)> _edgeDetectionPipelines;

    PushConstants     _toneMapConstants;

    U64 _lastDeltaTimeUS = 0u;
};

};  // namespace Divide
#endif
