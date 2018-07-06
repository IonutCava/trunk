#include "Headers/GLWrapper.h"
#include "Headers/glImmediateModeEmulation.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Scenes/Headers/SceneState.h"
#include "Core/Math/Headers/Transform.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"

#include "Hardware/Video/Headers/GFXDevice.h"

//Max number of frames before an unused primitive is deleted (default: 180 - 3 seconds at 60 fps)
const GLint GLIM_MAX_FRAMES_ZOMBIE_COUNT = 180;

GLint GL_API::FRAME_DRAW_CALLS = 0;
GLint GL_API::FRAME_DRAW_CALLS_PREV = 0;
GLuint64 GL_API::FRAME_DURATION_GPU = 0;
GLuint GL_API::FRAME_COUNT = 0;

#include <glim.h>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

namespace IMPrimitiveValidation{
    inline bool zombieCountMatch(glIMPrimitive* const priv){
        if(priv->_canZombify) return priv->zombieCounter() < GLIM_MAX_FRAMES_ZOMBIE_COUNT;
        else return true;
    }
}

void GL_API::beginFrame(){
#ifdef _DEBUG
    GLuint queryId = _queryID[_queryBackBuffer][0];
    glBeginQuery(GL_TIME_ELAPSED, queryId);
#endif
    GL_API::clearColor(DefaultColors::DIVIDE_BLUE(), 0, true);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT/* | GL_STENCIL_BUFFER_BIT*/);
    GL_API::registerDrawCall();
    SET_DEFAULT_STATE_BLOCK();
}

void GL_API::endFrame(){

    //Remove dead primitives in 3 steps (or we could automate this with shared_ptr?):
    //1) Partition the vector in 2 parts: valid objects first, zombie objects second
    vectorImpl<glIMPrimitive* >::iterator zombie = std::partition(_glimInterfaces.begin(),
                                                                  _glimInterfaces.end(),
                                                                  IMPrimitiveValidation::zombieCountMatch);
    //2) For every zombie object, free the memory it's using
    for( vectorImpl<glIMPrimitive *>::iterator i = zombie ; i != _glimInterfaces.end() ; ++i ){
        SAFE_DELETE(*i);
    }

    //3) Remove all the zombie objects once the memory is freed
    _glimInterfaces.erase(zombie, _glimInterfaces.end());

    //unbind all states (shaders, textures, buffers)
    clearStates(false,false,false,true);

    glfwSwapBuffers(Divide::GLUtil::_mainWindow);
    GL_API::FRAME_COUNT++;

#ifdef _DEBUG
    glEndQuery(GL_TIME_ELAPSED);

    GLuint query = GL_API::FRAME_COUNT % 4;
    
    _queryBackBuffer = query;
    _queryFrontBuffer = 3 - _queryBackBuffer;
    
#endif
    GL_API::FRAME_DRAW_CALLS_PREV = GL_API::FRAME_DRAW_CALLS;
    GL_API::FRAME_DRAW_CALLS = 0;
}

void GL_API::debugDraw(const SceneRenderState& sceneRenderState){
    drawDebugAxisInternal(sceneRenderState);

    _imShader->bind();

    NS_GLIM::GLIM_BATCH::s_bForceWireframe = false;

    for(glIMPrimitive* priv : _glimInterfaces){
        if(priv->paused())
            continue;
        if(!priv->_inUse && priv->_canZombify) {
            ++priv->_zombieCounter;
            continue;
        }

        if(!priv->_setupStates.empty()){
            priv->_setupStates();
        }else{
            SET_DEFAULT_STATE_BLOCK(true);
        }
        if(priv->_hasLines){
            glLineWidth(std::min(priv->_lineWidth, (F32)_lineWidthLimit));
        }

        bool texture = (priv->_texture != nullptr);

        if(texture){
            priv->_texture->Bind(0);
        }

        _imShader->Uniform("useTexture", texture);

        _imShader->uploadNodeMatrices();

        priv->renderBatch(priv->forceWireframe());

        if(priv->_hasLines) {
            glLineWidth(1.0f);
        }

        if(!priv->_resetStates.empty()){
            priv->_resetStates();
        }
        if(priv->_canZombify){
            priv->_inUse = false;
        }
    }
    _imShader->unbind();
}

void GL_API::flush(){
    clearStates(false,false,false,false);
}

void GL_API::drawBox3D(const vec3<GLfloat>& min,const vec3<GLfloat>& max, const mat4<GLfloat>& globalOffset){
    IMPrimitive* priv = getOrCreateIMPrimitive();

    priv->_hasLines = true;
    priv->_lineWidth = 4.0f;
    mat4<F32> offset(globalOffset);

    priv->setRenderStates(DELEGATE_BIND(&GL_API::setupLineState, this, offset,  GFX_DEVICE._defaultStateBlockHash, false),
                          DELEGATE_BIND(&GL_API::releaseLineState, this, false));
    priv->beginBatch();

    vec4<GLubyte> color(0,0,255,255);
    priv->attribute4ub("inColorData", color);

    priv->begin(LINE_LOOP);
        priv->vertex( vec3<GLfloat>(min.x, min.y, min.z) );
        priv->vertex( vec3<GLfloat>(max.x, min.y, min.z) );
        priv->vertex( vec3<GLfloat>(max.x, min.y, max.z) );
        priv->vertex( vec3<GLfloat>(min.x, min.y, max.z) );
    priv->end();

    priv->begin(LINE_LOOP);
        priv->vertex( vec3<GLfloat>(min.x, max.y, min.z) );
        priv->vertex( vec3<GLfloat>(max.x, max.y, min.z) );
        priv->vertex( vec3<GLfloat>(max.x, max.y, max.z) );
        priv->vertex( vec3<GLfloat>(min.x, max.y, max.z) );
    priv->end();

    priv->begin(LINES);
        priv->vertex( vec3<GLfloat>(min.x, min.y, min.z) );
        priv->vertex( vec3<GLfloat>(min.x, max.y, min.z) );
        priv->vertex( vec3<GLfloat>(max.x, min.y, min.z) );
        priv->vertex( vec3<GLfloat>(max.x, max.y, min.z) );
        priv->vertex( vec3<GLfloat>(max.x, min.y, max.z) );
        priv->vertex( vec3<GLfloat>(max.x, max.y, max.z) );
        priv->vertex( vec3<GLfloat>(min.x, min.y, max.z) );
        priv->vertex( vec3<GLfloat>(min.x, max.y, max.z) );
    priv->end();

    priv->endBatch();
}

void GL_API::setupLineState(const mat4<F32>& mat, I64 drawStateHash, const bool ortho){
    GFX_DEVICE.pushWorldMatrix(mat,true);
    SET_STATE_BLOCK(drawStateHash);

    if(ortho){
        GFX_DEVICE.setViewport(vec4<GLint>(_cachedResolution.width - 128, 0, 128, 128));
        Divide::GLUtil::_matrixMode(VIEW_MATRIX);
        Divide::GLUtil::_pushMatrix();
        Divide::GLUtil::_loadIdentity();
    }
}

void GL_API::releaseLineState(const bool ortho){
    GFX_DEVICE.popWorldMatrix();
    if(ortho){
        GFX_DEVICE.restoreViewport();
        Divide::GLUtil::_matrixMode(VIEW_MATRIX);
        Divide::GLUtil::_popMatrix();
    }
}

void GL_API::drawDebugAxisInternal(const SceneRenderState& sceneRenderState){
    if(!GFX_DEVICE.drawDebugAxis()) return;
    
    if(_axisPointsA.empty()){
        vec3<GLfloat> ORG(0,0,0);
        vec3<GLfloat> XP(1,0,0);
        vec3<GLfloat> YP(0,1,0);
        vec3<GLfloat> ZP(0,0,1);

        //Red X-axis
        _axisPointsA.push_back(ORG);
        _axisPointsB.push_back(XP);
        _axisColors.push_back(vec4<GLubyte>(255,0,0,255));

        //Green Y-axis
        _axisPointsA.push_back(ORG);
        _axisPointsB.push_back(YP);
        _axisColors.push_back(vec4<GLubyte>(0,255,0,255));

        //Blue Z-axis
        _axisPointsA.push_back(ORG);
        _axisPointsB.push_back(ZP);
        _axisColors.push_back(vec4<GLubyte>(0,0,255,255));
    }

    const glm::mat4& viewMatrix = Divide::GLUtil::_viewMatrix.top();
    vec3<GLfloat> eyeVector = -(sceneRenderState.getCameraConst().getViewDir() * 2);
    mat4<F32> offset(eyeVector, vec3<F32>(0.0f), vec3<F32>(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]));
    drawLines(_axisPointsA, _axisPointsB, _axisColors, offset, true, true);
}

void GL_API::drawLines(const vectorImpl<vec3<GLfloat> >& pointsA,
                       const vectorImpl<vec3<GLfloat> >& pointsB,
                       const vectorImpl<vec4<GLubyte> >& colors,
                       const mat4<GLfloat>& globalOffset,
                       const bool orthoMode,
                       const bool disableDepth){
    // We need a perfect pair of point A's to point B's
    // We need a color for each line, even if it is the same one
    if(pointsA.size() != pointsB.size() || pointsA.size() != colors.size()) return;
    

    IMPrimitive* priv = getOrCreateIMPrimitive();

    priv->_hasLines = true;
    priv->_lineWidth = std::min((F32)_lineWidthLimit, 5.0f);

    if(!priv->hasRenderStates()){
        priv->setRenderStates(DELEGATE_BIND(&GL_API::setupLineState, this, globalOffset, disableDepth ? GFX_DEVICE._defaultStateNoDepthHash : GFX_DEVICE._defaultStateBlockHash,orthoMode),
                              DELEGATE_BIND(&GL_API::releaseLineState, this, orthoMode));
    }

    priv->beginBatch();

    priv->attribute4ub("inColorData", colors[0]);

    priv->begin(LINES);
    for(GLushort i = 0; i < pointsA.size(); i++){
        priv->attribute4ub("inColorData", colors[i]);
        priv->vertex( pointsA[i] );
        priv->vertex( pointsB[i] );
    }

    priv->end();
    priv->endBatch();
}

void GL_API::drawPoints(GLuint numPoints) {
    GL_API::setActiveVAO(_pointDummyVAO);
    glDrawArrays(GL_POINTS, 0, numPoints);
    GL_API::registerDrawCall();
}