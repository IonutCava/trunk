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

    inline PreRenderOperator& getOperator(FilterType type) {
        PreRenderOperator* op = _operators[getOperatorIndex(type)];
        assert(op != nullptr);

        return *op;
    }

   private:
    inline U32 getOperatorIndex(FilterType type) const {
        switch (type) {
            case FilterType::FILTER_SS_REFLECTIONS:
                return 0;
            case FilterType::FILTER_SS_AMBIENT_OCCLUSION:
                return 1;
            case FilterType::FILTER_DEPTH_OF_FIELD:
                return 2;
            case FilterType::FILTER_MOTION_BLUR:
                return 3;
            case FilterType::FILTER_BLOOM_TONEMAP:
                return 4;
            case FilterType::FILTER_SS_ANTIALIASING:
                return 5;
        };

        return 6; //FilterType::FILTER_LUT_CORECTION;
    }

  private:
    std::array<PreRenderOperator*, 8> _operators;
    PreRenderOperator* _debugOperator;
    Framebuffer* _postFXOutput;
    Framebuffer* _renderTarget;
};

};  // namespace Divide
#endif
