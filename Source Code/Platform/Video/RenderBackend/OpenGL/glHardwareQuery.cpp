#include "stdafx.h"

#include "Headers/glHardwareQuery.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"

namespace Divide {

glHardwareQuery::glHardwareQuery(GFXDevice& context) 
    : glObject(glObjectType::TYPE_QUERY, context),
      _enabled(false),
      _queryID(0u)
{
}

glHardwareQuery::~glHardwareQuery()
{
    assert(_queryID == 0u);
}

void glHardwareQuery::create() {
    destroy();
    glGenQueries(1, &_queryID);
}

void glHardwareQuery::destroy() {
    if (_queryID != 0u) {
        glDeleteQueries(1, &_queryID);
    }
    _queryID = 0u;
}

glHardwareQueryRing::glHardwareQueryRing(GFXDevice& context, U32 queueLength, U32 id)
    : RingBufferSeparateWrite(queueLength, true),
      _context(context),
      _id(id)
{
    _queries.reserve(queueLength);
    resize(queueLength);
}

glHardwareQueryRing::~glHardwareQueryRing()
{
    for (glHardwareQuery& query : _queries) {
        query.destroy();
    }
    _queries.clear();
}

glHardwareQuery& glHardwareQueryRing::readQuery() {
    return _queries[queueReadIndex()];
}

glHardwareQuery& glHardwareQueryRing::writeQuery() {
    return _queries[queueWriteIndex()];
}

void glHardwareQueryRing::resize(I32 queueLength) noexcept {
    RingBufferSeparateWrite::resize(queueLength);

    const I32 crtCount = to_I32(_queries.size());
    if (queueLength < crtCount) {
        while (queueLength < to_I32(crtCount)) {
            _queries.back().destroy();
            _queries.pop_back();
        }
    } else {
        const I32 countToAdd = queueLength - crtCount;

        for (I32 i = 0; i < countToAdd; ++i) {
            _queries.emplace_back(_context);
            _queries.back().create();
        }
    }
}

}; //namespace Divide