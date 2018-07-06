#include "stdafx.h"

#include "Headers/glHardwareQuery.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"

namespace Divide {

glHardwareQuery::glHardwareQuery() 
    : glObject(glObjectType::TYPE_QUERY),
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

glHardwareQueryRing::glHardwareQueryRing(U32 queueLength)
    : RingBuffer(queueLength)
{
    _queries.resize(queueLength);
}

glHardwareQuery& glHardwareQueryRing::readQuery() {
    return _queries[queueReadIndex()];
}

glHardwareQuery& glHardwareQueryRing::writeQuery() {
    return _queries[queueWriteIndex()];
}

void glHardwareQueryRing::initQueries() {
    for (glHardwareQuery& query : _queries) {
        query.create();
        assert(query.getID() != 0 && "glHardwareQueryRing error: Invalid performance counter query ID!");
        // Initialize an initial time query as it solves certain issues with
        // consecutive queries later
        glBeginQuery(GL_TIME_ELAPSED, query.getID());
        glEndQuery(GL_TIME_ELAPSED);
        // Wait until the results are available
        GLint stopTimerAvailable = 0;
        while (!stopTimerAvailable) {
            glGetQueryObjectiv(query.getID(), GL_QUERY_RESULT_AVAILABLE, &stopTimerAvailable);
        }
    }
}
}; //namespace Divide