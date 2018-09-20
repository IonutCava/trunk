#include "stdafx.h"

#include "Headers/glHardwareQuery.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"

namespace Divide {

glHardwareQuery::glHardwareQuery(GFXDevice& context) 
    : glObject(glObjectType::TYPE_QUERY, context),
      _enabled(false),
      _queryID(0)
{
}

glHardwareQuery::~glHardwareQuery()
{
    destroy();
}

void glHardwareQuery::create() {
    destroy();
    glGenQueries(1, &_queryID);
}

void glHardwareQuery::destroy() {
    if (_queryID != 0) {
        glDeleteQueries(1, &_queryID);
    }
    _queryID = 0;
}

glHardwareQueryRing::glHardwareQueryRing(GFXDevice& context, U32 queueLength, U32 id)
    : RingBufferSeparateWrite(queueLength),
      _context(context),
      _id(id)
{
    _queries.reserve(queueLength);
    for (U32 i = 0; i < queueLength; ++i) {
        _queries.push_back(std::make_shared<glHardwareQuery>(context));
        _queries.back()->create();
    }
}

glHardwareQueryRing::~glHardwareQueryRing()
{
    _queries.clear();
}

glHardwareQuery& glHardwareQueryRing::readQuery() {
    return *_queries[queueReadIndex()];
}

glHardwareQuery& glHardwareQueryRing::writeQuery() {
    return *_queries[queueWriteIndex()];
}

void glHardwareQueryRing::resize(U32 queueLength) {
    RingBufferSeparateWrite::resize(queueLength);

    while (queueLength < to_U32(_queries.size())) {
        _queries.pop_back();
    }
    
    while (queueLength > to_U32(_queries.size())) {
        _queries.push_back(std::make_shared<glHardwareQuery>(_context));
        _queries.back()->create();
    }
}

}; //namespace Divide