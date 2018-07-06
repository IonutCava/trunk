#include "Headers/GLWrapper.h"
#include "Headers/glImmediateModeEmulation.h"

#include "GUI/Headers/GUIText.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Rendering/Headers/Frustum.h"
#include "Utility/Headers/ImageTools.h"
#include "Core/Math/Headers/Transform.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"

#include "Hardware/Video/Headers/GFXDevice.h"

//Max number of frames before an unused primitive is deleted (default: 180 - 3 seconds at 60 fps)
#define GLIM_MAX_FRAMES_ZOMBIE_COUNT 180

#ifndef FTGL_LIBRARY_STATIC
#define FTGL_LIBRARY_STATIC
#endif
#include <FTGL/ftgl.h>
#include <glim/include/glim.h>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

namespace IMPrimitiveValidation{
    inline bool zombieCountMatch(glIMPrimitive* const priv){
        if(priv->_canZombify) return priv->zombieCounter() < GLIM_MAX_FRAMES_ZOMBIE_COUNT;
        else return true;
    }
}

void GL_API::beginFrame(){
    GLCheck(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
    GL_API::clearColor(DefaultColors::DIVIDE_BLUE());
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

    glfwSwapBuffers(Divide::GL::_mainWindow);
}

void GL_API::debugDraw(){
    drawDebugAxisInternal();

    _imShader->bind();
    _imShader->Uniform("tex",0);

    NS_GLIM::GLIM_BATCH::s_bForceWireframe = false;

    for_each(glIMPrimitive* priv, _glimInterfaces){
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
            GLCheck(glLineWidth(priv->_lineWidth));
        }

        bool texture = (priv->_texture != NULL);

        if(texture){
            priv->_texture->Bind(0);
        }

        _imShader->Uniform("useTexture", texture);
        _imShader->uploadNodeMatrices();

        priv->renderBatch(priv->forceWireframe());

        if(priv->_hasLines) {
            GLCheck(glLineWidth(1.0f));
        }
        if(priv->_texture){
            priv->_texture->Unbind(0);
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

void GL_API::renderInViewport(const vec4<GLuint>& rect, boost::function0<void> callback){
    setViewport(rect);
    callback();
    restoreViewport();
}

void GL_API::drawBox3D(const vec3<GLfloat>& min,const vec3<GLfloat>& max, const mat4<GLfloat>& globalOffset){
    IMPrimitive* priv = getOrCreateIMPrimitive();

    priv->_hasLines = true;
    priv->_lineWidth = 4.0f;
    mat4<F32> offset(globalOffset);

    priv->setRenderStates(DELEGATE_BIND(&GL_API::setupLineState, this, offset,  GFX_DEVICE._defaultStateBlock, false),
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

void GL_API::setupLineState(const mat4<F32>& mat, RenderStateBlock* const drawState,const bool ortho){
    GFX_DEVICE.pushWorldMatrix(mat,false);
    SET_STATE_BLOCK(drawState,true);

    if(ortho){
        setViewport(vec4<U32>(_cachedResolution.width - 128, 0, 128, 128));
        Divide::GL::_matrixMode(VIEW_MATRIX);
        Divide::GL::_pushMatrix();
        Divide::GL::_loadIdentity();
    }
}

void GL_API::releaseLineState(const bool ortho){
    GFX_DEVICE.popWorldMatrix();
    if(ortho){
        restoreViewport();
        Divide::GL::_matrixMode(VIEW_MATRIX);
        Divide::GL::_popMatrix();
    }
}

void GL_API::drawDebugAxisInternal(){
    if(!GFX_DEVICE.drawDebugAxis()) return;
    static bool axisData = false;
    if(!axisData){
        vec3<GLfloat> ORG(0,0,0);
        vec3<GLfloat> XP(1,0,0);
        vec3<GLfloat> YP(0,1,0);
        vec3<GLfloat> ZP(0,0,1);

        //Red X-axis
        _pointsA.push_back(ORG);
        _pointsB.push_back(XP);
        _colors.push_back(vec4<GLubyte>(255,0,0,255));

        //Green Y-axis
        _pointsA.push_back(ORG);
        _pointsB.push_back(YP);
        _colors.push_back(vec4<GLubyte>(0,255,0,255));

        //Blue Z-axis
        _pointsA.push_back(ORG);
        _pointsB.push_back(ZP);
        _colors.push_back(vec4<GLubyte>(0,0,255,255));
        axisData = true;
    }

    vec3<GLfloat> eyeVector = - (Divide::GL::_currentViewDirection.top() * 2);
    const glm::mat4& viewMatrix = Divide::GL::_viewMatrix.top();

    drawLines(_pointsA, _pointsB, _colors,
              mat4<GLfloat>(glm::value_ptr(glm::lookAt(glm::vec3(eyeVector.x, eyeVector.y, eyeVector.z),
                                                       glm::vec3(0.0f),
                                                       glm::vec3(viewMatrix[0][1],
                                                                 viewMatrix[1][1],
                                                                 viewMatrix[2][1])))),
              true,
              true);
}

void GL_API::drawLines(const vectorImpl<vec3<GLfloat> >& pointsA,
                       const vectorImpl<vec3<GLfloat> >& pointsB,
                       const vectorImpl<vec4<GLubyte> >& colors,
                       const mat4<GLfloat>& globalOffset,
                       const bool orthoMode,
                       const bool disableDepth){
    /// We need a perfect pair of point A's to point B's
    if(pointsA.size() != pointsB.size()) return;
    /// We need a color for each line, even if it is the same one
    if(pointsA.size() != colors.size()) return;

    IMPrimitive* priv = getOrCreateIMPrimitive();
    mat4<F32> offset(globalOffset);

    priv->_hasLines = true;
    priv->_lineWidth = 5.0f;
    if(!priv->hasRenderStates()){
        priv->setRenderStates(DELEGATE_BIND(&GL_API::setupLineState, this, offset, disableDepth ? _defaultStateNoDepth : GFX_DEVICE._defaultStateBlock,orthoMode),
                              DELEGATE_BIND(&GL_API::releaseLineState, this, orthoMode));
    }
    priv->beginBatch();

    vec4<GLubyte> color(255,0,0,255);
    priv->attribute4ub("inColorData", color);

    priv->begin(LINES);
    vec3<GLfloat> vertA, vertB;
    for(GLushort i = 0; i < pointsA.size(); i++){
        color.set(colors[i].r, colors[i].g, colors[i].b,colors[i].a);
        vertA.set(pointsA[i].x,pointsA[i].y,pointsA[i].z);
        vertB.set(pointsB[i].x,pointsB[i].y,pointsB[i].z);
        priv->attribute4ub("inColorData", color);
        priv->vertex( vertA );
        priv->vertex( vertB );
    }

    priv->end();
    priv->endBatch();
}

void GL_API::drawText(const std::string& text, const GLint width, const std::string& fontName, const GLfloat fontSize){
    setActiveVAO(0);

    FTFont* font = getFont(fontName);
    if(!font) return;
    if(!FLOAT_COMPARE(_prevSizeNode,fontSize)){
        if (!font->FaceSize(fontSize)) {
            ERROR_FN(Locale::get("ERROR_FONT_HEIGHT"),fontSize);
        }
        _prevSizeNode = fontSize;
    }
    if(_prevWidthNode != width){
        GLCheck(glLineWidth(width));
        _prevWidthNode = width;
    }
    font->Render(text.c_str());
}

void GL_API::drawText(const std::string& text, const GLint width, const vec2<GLint> position, const std::string& fontName, const GLfloat fontSize){
    FTFont* font = getFont(fontName);
    if(!font) return;

    if(!FLOAT_COMPARE(_prevSizeString,fontSize)){
        if (!font->FaceSize(fontSize)) {
            ERROR_FN(Locale::get("ERROR_FONT_HEIGHT"),fontSize);
        }
        _prevSizeString = fontSize;
    }

    if(_prevWidthString != width){
        GLCheck(glLineWidth(width));
        _prevWidthString = width;
    }

    GLfloat y = _cachedResolution.height - position.y;
    if(!FLOAT_COMPARE(_prevPointString->X(),position.x)){
        _prevPointString->X(position.x);
    }

    if(!FLOAT_COMPARE(_prevPointString->Y(), y)){
        _prevPointString->Y(y);
    }

    font->Render(text.c_str(),text.length(),*_prevPointString);
}

//Save the area designated by the rectangle "rect" to a TGA file
void GL_API::Screenshot(char *filename, const vec4<GLfloat>& rect){
    // compute width and heidth of the image
    GLushort w = rect.z - rect.x; //maxX - minX
    GLushort h = rect.w - rect.y; //maxY - minY

    // allocate memory for the pixels
    GLubyte *imageData = New GLubyte[w * h * 4];
    // read the pixels from the frame buffer
    GLCheck(glReadPixels(rect.x,rect.y,rect.z,rect.w,GL_RGBA,GL_UNSIGNED_BYTE, (GLvoid*)imageData));
    //save to file
    ImageTools::SaveSeries(filename,vec2<GLushort>(w,h),32,imageData);
}