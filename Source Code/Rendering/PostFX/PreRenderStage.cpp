#include "Headers/PreRenderStage.h"
#include "Headers/PreRenderOperator.h"

namespace Divide {

PreRenderStage::~PreRenderStage() { MemoryManager::DELETE_VECTOR(_operators); }

void PreRenderStage::execute() {
    for (PreRenderOperator* op : _operators) {
        op->operation();
    }
}

void PreRenderStage::reshape(U16 width, U16 height) {
    for (PreRenderOperator* op : _operators) {
        op->reshape(width, height);
    }
}
};