#include "stdafx.h"

#include "Headers/glVAOPool.h"
#include "Core/Headers/StringHelper.h"
#include "Platform/Headers/PlatformRuntime.h"

namespace Divide {
namespace GLUtil {

namespace {
    const U32 g_numWarmupVAOs = 25u;
};

glVAOPool::glVAOPool()
{

}

glVAOPool::~glVAOPool()
{
    destroy();
}

void glVAOPool::init(U32 capacity) {
    destroy();

    GLuint warmupVAOs[g_numWarmupVAOs] = { { 0u } };
    glCreateVertexArrays(g_numWarmupVAOs, warmupVAOs);

    _pool.resize(capacity, std::make_pair(0, false));
    for (std::pair<GLuint, bool>& entry : _pool) {
        glCreateVertexArrays(1, &entry.first);
        if (Config::ENABLE_GPU_VALIDATION) {
            glObjectLabel(GL_VERTEX_ARRAY,
                entry.first,
                -1,
                Util::StringFormat("DVD_VAO_%d", entry.first).c_str());
        }
    }


    glDeleteVertexArrays(g_numWarmupVAOs, warmupVAOs);
}

void glVAOPool::destroy() {
    for (std::pair<GLuint, bool>& entry : _pool) {
        glDeleteVertexArrays(1, &entry.first);
    }
    _pool.clear();
}

GLuint glVAOPool::allocate() {
    for (std::pair<GLuint, bool>& entry : _pool) {
        if (entry.second == false) {
            entry.second = true;
            return entry.first;
        }
    }

    DIVIDE_UNEXPECTED_CALL();
    return 0;
}

void glVAOPool::allocate(U32 count, GLuint* vaosOUT) {
    for (U32 i = 0; i < count; ++i) {
        vaosOUT[i] = allocate();
    }
}

void glVAOPool::deallocate(GLuint& vao) {
    assert(Runtime::isMainThread());

    vectorImpl<std::pair<GLuint, bool>>::iterator it;
    it = std::find_if(std::begin(_pool),
        std::end(_pool),
        [vao](std::pair<GLuint, bool>& entry) {
        return entry.first == vao;
    });

    assert(it != std::cend(_pool));
    // We don't know what kind of state we may have in the current VAO so delete it and create a new one.
    glDeleteVertexArrays(1, &(it->first));
    glCreateVertexArrays(1, &(it->first));

    if (Config::ENABLE_GPU_VALIDATION) {
        glObjectLabel(GL_VERTEX_ARRAY,
            it->first,
            -1,
            Util::StringFormat("DVD_VAO_%d", it->first).c_str());
    }
    it->second = false;
    vao = 0;
}


}; //namespace GLUtil
}; //namespace Divide