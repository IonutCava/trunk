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
      _id(id),
      _needRefresh(true)
{
    _queries.reserve(queueLength);
    for (U32 i = 0; i < queueLength; ++i) {
        _queries.push_back(std::make_shared<glHardwareQuery>(context));
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

void glHardwareQueryRing::initQueries() {
    if (_needRefresh) {
        for (std::shared_ptr<glHardwareQuery>& query : _queries) {
            query->create();
            assert(query->getID() != 0 && "glHardwareQueryRing error: Invalid performance counter query ID!");
            // Initialize an initial time query as it solves certain issues with
            // consecutive queries later
            glBeginQuery(GL_TIME_ELAPSED, query->getID());
            glEndQuery(GL_TIME_ELAPSED);
            // Wait until the results are available
            GLint stopTimerAvailable = 0;
            while (!stopTimerAvailable) {
                glGetQueryObjectiv(query->getID(), GL_QUERY_RESULT_AVAILABLE, &stopTimerAvailable);
            }
        }
        _needRefresh = false;
    }
}

void glHardwareQueryRing::resize(U32 queueLength) {
    RingBufferSeparateWrite::resize(queueLength);
    _queries.resize(0);
    _queries.reserve(queueLength);
    for (U32 i = 0; i < queueLength; ++i) {
        _queries.push_back(std::make_shared<glHardwareQuery>(_context));
    }
    _needRefresh = true;
    initQueries();
}

}; //namespace Divide