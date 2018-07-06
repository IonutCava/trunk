#ifndef _PRE_RenderStage_H_
#define _PRE_RenderStage_H_

#include "PreRenderOperator.h"

namespace Divide {

class PreRenderBatch {
   public:
    PreRenderBatch();
    ~PreRenderBatch();

    void init(Framebuffer* renderTarget);
    void destroy();

    void idle();
    void execute(U32 filterMask);
    void reshape(U16 width, U16 height);

    void bindOutput(U8 slot);

    inline void toggleAdaptivExposure(const bool state) {
        _adaptiveExposureControl = state;
    }

    inline PreRenderOperator& getOperator(FilterType type) {
        const OperatorBatch& batch = _operators[to_uint(getOperatorSpace(type))];
        OperatorBatch::const_iterator it =
            std::find_if(std::cbegin(batch), std::cend(batch),
                [type](PreRenderOperator* op) {
            return op->operatorType() == type;
        });
        assert(it != std::cend(batch));
        return *(*it);
    }

    inline const PreRenderOperator& getOperator(FilterType type) const {
        const OperatorBatch& batch = _operators[to_uint(getOperatorSpace(type))];
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

  private:
    typedef vectorImpl<PreRenderOperator*> OperatorBatch;
    OperatorBatch _operators[to_const_uint(FilterSpace::COUNT)];

    bool _adaptiveExposureControl;

    PreRenderOperator* _debugOperator;
    Framebuffer* _postFXOutput;
    Framebuffer* _renderTarget;
    Framebuffer* _previousLuminance;
    Framebuffer* _currentLuminance;
    std::shared_ptr<ShaderProgram> _toneMap;
    std::shared_ptr<ShaderProgram> _toneMapAdaptive;
    std::shared_ptr<ShaderProgram> _luminanceCalc;
};

};  // namespace Divide
#endif
