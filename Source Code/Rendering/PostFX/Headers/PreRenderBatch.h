#ifndef _PRE_RenderStage_H_
#define _PRE_RenderStage_H_

#include "PreRenderOperator.h"

namespace Divide {

class ResourceCache;
class PreRenderBatch {
   public:
    PreRenderBatch(GFXDevice& context, ResourceCache& cache);
    ~PreRenderBatch();

    void init(RenderTargetID renderTarget);
    void destroy();

    void idle(const Configuration& config);
    void prepare(const Camera& camera, U16 filterStack, GFX::CommandBuffer& bufferInOut);
    void execute(const Camera& camera, U16 filterStack, GFX::CommandBuffer& bufferInOut);
    void reshape(U16 width, U16 height);

    void onFilterEnabled(FilterType filter);
    void onFilterDisabled(FilterType filter);

    TextureData getOutput();

    RenderTargetHandle inputRT() const;
    RenderTargetHandle& outputRT();

    inline void toggleAdaptiveExposure(const bool state) {
        _adaptiveExposureControl = state;
    }

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

   private:

    inline FilterSpace getOperatorSpace(FilterType type) const {
        switch(type) {
            case FilterType::FILTER_SS_ANTIALIASING :
                return FilterSpace::FILTER_SPACE_LDR;
        }

        return FilterSpace::FILTER_SPACE_HDR;
    }

    void onFilterToggle(FilterType type, const bool state);
  private:
    typedef vector<PreRenderOperator*> OperatorBatch;
    OperatorBatch _operators[to_base(FilterSpace::COUNT)];

    GFXDevice& _context;
    ResourceCache& _resCache;

    bool _adaptiveExposureControl;

    RenderTargetID     _renderTarget;
    PreRenderOperator* _debugOperator;
    RenderTargetHandle _postFXOutput;
    RenderTargetHandle _previousLuminance;
    RenderTargetHandle _currentLuminance;
    ShaderProgram_ptr _toneMap;
    ShaderProgram_ptr _toneMapAdaptive;
    ShaderProgram_ptr _luminanceCalc;
    PushConstants     _toneMapConstants;
};

};  // namespace Divide
#endif
