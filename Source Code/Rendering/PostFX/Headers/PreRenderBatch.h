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
    PreRenderBatch(GFXDevice& context, PostFX& parent, ResourceCache* cache, RenderTargetID renderTarget);
    ~PreRenderBatch();

    PostFX& parent() const noexcept { return _parent; }

    void idle(const Configuration& config);
    void update(const U64 deltaTimeUS);

    void prepare(const Camera& camera, U32 filterStack, GFX::CommandBuffer& bufferInOut);
    void execute(const Camera& camera, U32 filterStack, GFX::CommandBuffer& bufferInOut);
    void reshape(U16 width, U16 height);

    void onFilterEnabled(FilterType filter);
    void onFilterDisabled(FilterType filter);

    TextureData getOutput();

    RenderTargetHandle& outputRT() noexcept;

    RenderTargetHandle inputRT() const noexcept;
    RenderTargetHandle edgesRT() const noexcept;
    RenderTargetHandle luminanceRT() const noexcept;

    inline PreRenderOperator* getOperator(FilterType type) {
        const FilterSpace fSpace = getOperatorSpace(type);
        if (fSpace == FilterSpace::COUNT) {
            return nullptr;
        }

        const OperatorBatch& batch = _operators[to_U32(fSpace)];
        auto it = std::find_if(std::cbegin(batch), std::cend(batch), [type](const std::unique_ptr<PreRenderOperator>& op) {
                                    return op->operatorType() == type;
                              });

        assert(it != std::cend(batch));
        return (*it).get();
    }

    inline const PreRenderOperator* getOperator(FilterType type) const {
        const FilterSpace fSpace = getOperatorSpace(type);
        if (fSpace == FilterSpace::COUNT) {
            return nullptr;
        }

        const OperatorBatch& batch = _operators[to_U32(getOperatorSpace(type))];
        auto it = std::find_if(std::cbegin(batch), std::cend(batch), [type](const std::unique_ptr<PreRenderOperator>& op) {
                                    return op->operatorType() == type;
                              });
        assert(it != std::cend(batch));
        return (*it).get();
    }


    PROPERTY_RW(ToneMapParams, toneMapParams, {});
    PROPERTY_RW(F32, edgeDetectionThreshold, 0.1f);
    PROPERTY_RW(bool,  adaptiveExposureControl, true);
    PROPERTY_RW(EdgeDetectionMethod, edgeDetectionMethod, EdgeDetectionMethod::Luma);

   private:

    inline FilterSpace getOperatorSpace(FilterType type) const noexcept {
        // ToDo: Always keep this up-to-date with every filter we add
        switch(type) {
            case FilterType::FILTER_SS_ANTIALIASING :
                return FilterSpace::FILTER_SPACE_LDR;

            case FilterType::FILTER_SS_AMBIENT_OCCLUSION:
            case FilterType::FILTER_DEPTH_OF_FIELD:
            case FilterType::FILTER_BLOOM:
                return FilterSpace::FILTER_SPACE_HDR;

            default: break;
        }

        return FilterSpace::COUNT;
    }

    void onFilterToggle(FilterType type, const bool state);

    void init();

    bool operatorsReady() const;
  private:
    using OperatorBatch = vectorEASTL<std::unique_ptr<PreRenderOperator>>;
    std::array<OperatorBatch, to_base(FilterSpace::COUNT)> _operators;

    GFXDevice& _context;
    PostFX&    _parent;
    ResourceCache* _resCache = nullptr;

    RenderTargetID     _renderTarget = RenderTargetUsage::SCREEN;
    ShaderBuffer*      _histogramBuffer = nullptr;
    RenderTargetHandle _postFXOutput;
    RenderTargetHandle _sceneEdges;
    RenderTargetHandle _currentLuminance;
    ShaderProgram_ptr _toneMap = nullptr;
    ShaderProgram_ptr _toneMapAdaptive = nullptr;
    ShaderProgram_ptr _createHistogram = nullptr;
    ShaderProgram_ptr _averageHistogram = nullptr;
    std::array<ShaderProgram_ptr, to_base(EdgeDetectionMethod::COUNT)> _edgeDetection = {};
    std::array<Pipeline*, to_base(EdgeDetectionMethod::COUNT)> _edgeDetectionPipelines = {};

    PushConstants     _toneMapConstants;

    U64 _lastDeltaTimeUS = 0u;
};

};  // namespace Divide
#endif
