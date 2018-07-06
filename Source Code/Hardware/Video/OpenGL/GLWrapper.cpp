#include "Headers/GLWrapper.h"
#include "Headers/glRenderStateBlock.h"

#include "GUI/Headers/GUIText.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Core/Headers/ParamHandler.h"
#include "Rendering/Headers/Frustum.h"
#include "Utility/Headers/ImageTools.h"
#include "Managers/Headers/ShaderManager.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"

#include "Hardware/Video/OpenGL/Buffers/FrameBufferObject/Headers/glTextureArrayBufferObject.h"
#include "Hardware/Video/OpenGL/Buffers/FrameBufferObject/Headers/glDepthArrayBufferObject.h"
#include "Hardware/Video/OpenGL/Buffers/FrameBufferObject/Headers/glMSTextureBufferObject.h"
#include "Hardware/Video/OpenGL/Buffers/FrameBufferObject/Headers/glTextureBufferObject.h"
#include "Hardware/Video/OpenGL/Buffers/FrameBufferObject/Headers/glDeferredBufferObject.h"
#include "Hardware/Video/OpenGL/Buffers/FrameBufferObject/Headers/glDepthBufferObject.h"
#include "Hardware/Video/OpenGL/Buffers/VertexBufferObject/Headers/glVertexArrayObject.h"
#include "Hardware/Video/OpenGL/Buffers/PixelBufferObject/Headers/glPixelBufferObject.h"

#define FTGL_LIBRARY_STATIC
#include <FTGL/ftgl.h>

#include <glim.h>
IMPrimitive::IMPrimitive() : _texture(NULL),
							 _hasLines(false),
							 _inUse(false),
							 _lineWidth(2.0f),
							 _zombieCounter(0){
	_imInterface = New NS_GLIM::GLIM_BATCH();
}

IMPrimitive::~IMPrimitive() {
	SAFE_DELETE(_imInterface);
	SAFE_DELETE(_texture);
}

namespace IMPrimitiveValidation{
	bool zombieCountMatch(const IMPrimitive* priv){	return (priv->_zombieCounter >= 10);}
	bool isValid(const IMPrimitive* priv){ return !priv->_inUse; }
}

GLuint GL_API::_activeShaderId = 0;
GLuint GL_API::_activeVBOId = 0;
GLuint GL_API::_activeVAOId = 0;
GLuint GL_API::_activeTextureUnit = 0;
GLuint GL_API::_activeClientTextureUnit = 0;

void GL_API::clearStates(bool skipShader, bool skipTextures, bool skipBuffers){
    if(_activeShaderId != 0 && !skipShader) {
        ShaderManager::getInstance().unbind();
    }
    if(_activeTextureUnit != 0 && !skipTextures){
        GLCheck(glActiveTexture(GL_TEXTURE0));
        _activeTextureUnit = 0;
    }
    if(_activeClientTextureUnit != 0 && !skipTextures){
        GLCheck(glClientActiveTexture(GL_TEXTURE0));
        _activeClientTextureUnit = 0;
    }
    if(_activeVBOId != 0 && !skipBuffers){
        GLCheck(glBindBuffer(GL_ARRAY_BUFFER, 0));
        GLCheck(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
        _activeVBOId = 0;
    }
    if(_activeVAOId != 0 && !skipBuffers){
        GLCheck(glBindVertexArray(0));
        _activeVAOId = 0;
    }
}

void GL_API::flush(){

	_imShader->bind();

	for_each(IMPrimitive* priv, _glimInterfaces){
		if(!priv->_inUse) {
			++priv->_zombieCounter;
			continue;
		}
		if(!priv->_setupStates.empty()){
			priv->_setupStates();
		}
		if(priv->_texture){
			priv->_texture->Bind(0);
			_imShader->Uniform("texture",0);
			_imShader->Uniform("useTexture", true);
		}else{
			_imShader->Uniform("useTexture", false);
		}
		if(priv->_hasLines){
			GLCheck(glPushAttrib(GL_LINE_BIT));
			GLCheck(glLineWidth(priv->_lineWidth));
		}

		priv->_imInterface->RenderBatch();

		if(priv->_hasLines){
			GLCheck(glPopAttrib());
		}
		if(priv->_texture){
			priv->_texture->Unbind(0);
		}
		if(!priv->_resetStates.empty()){
			priv->_resetStates();
		}
		priv->_inUse = false;
	}
	_imShader->unbind();

	///Remove dead primitives in 3 steps (or we could automate this with shared_ptr?):
	///1) Partition the vector in 2 parts: valid objects first, zombie objects second
	vectorImpl<IMPrimitive* >::iterator zombie = std::partition(_glimInterfaces.begin(),
							  							        _glimInterfaces.end(),
																IMPrimitiveValidation::zombieCountMatch);
	///2) For every zombie object, free the memory it's using
	for( vectorImpl<IMPrimitive *>::iterator i = zombie ; i != _glimInterfaces.end() ; ++i ){
		SAFE_DELETE(*i);
	}
	///3) Remove all the zombie objects once the memory is freed
	_glimInterfaces.erase(zombie, _glimInterfaces.end());
}

#pragma message("ToDo: convert to OpenGL 3.2 and GLSL 1.5 standards. No more matrix queries to GPU - Ionut")
void GL_API::getMatrix(MATRIX_MODE mode, mat4<GLfloat>& mat){
	switch(mode){
		case MODEL_VIEW_MATRIX:
			GLCheck(glGetFloatv( GL_MODELVIEW_MATRIX, mat.mat ));	
			//mat = _modelViewMatrix.top();
			break;
		case PROJECTION_MATRIX:
			GLCheck(glGetFloatv( GL_PROJECTION_MATRIX, mat.mat ));	
			//mat = _projectionMatrix.top();
			break;
	};
}

void GL_API::clearBuffers(GLushort buffer_mask){

	GLint buffers = 0;
	if((buffer_mask & GFXDevice::COLOR_BUFFER ) == GFXDevice::COLOR_BUFFER) buffers |= GL_COLOR_BUFFER_BIT;
	if((buffer_mask & GFXDevice::DEPTH_BUFFER) == GFXDevice::DEPTH_BUFFER) buffers |= GL_DEPTH_BUFFER_BIT;
	if((buffer_mask & GFXDevice::STENCIL_BUFFER) == GFXDevice::STENCIL_BUFFER) buffers |= GL_STENCIL_BUFFER_BIT;
	GLCheck(glClear(buffers));
}

void GL_API::enableFog(FogMode mode, GLfloat density, GLfloat* color, GLfloat startDist, GLfloat endDist){
    switch(mode){
        default:
        case FOG_NONE: break;
        case FOG_EXP:
            GLCheck(glFogi (GL_FOG_MODE, GL_EXP)); 
            break;
        case FOG_EXP2:
	        GLCheck(glFogi (GL_FOG_MODE, GL_EXP2)); 
            break;
        case FOG_LINEAR:
            GLCheck(glFogi (GL_FOG_MODE, GL_LINEAR)); 
            break;
    }
	GLCheck(glFogfv(GL_FOG_COLOR, color)); 
	GLCheck(glFogf (GL_FOG_DENSITY, density)); 
	GLCheck(glFogf (GL_FOG_START, startDist));
	GLCheck(glFogf (GL_FOG_END, endDist));
}

void GL_API::drawTextToScreen(GUIElement* const textElement){
	GUIText* text = dynamic_cast<GUIText* >(textElement);
	assert(text != NULL);

	const char* fontName = text->_font.c_str();
	FontCache::iterator itr = _fonts.find(fontName);
	if(itr == _fonts.end()){
		std::string fontPath = ParamHandler::getInstance().getParam<std::string>("assetsLocation") + "/";
		fontPath += "GUI/";
		fontPath += "fonts/";
		fontPath += text->_font;
		FTFont* tempFont = New FTGLBitmapFont(fontPath.c_str());
		if (!tempFont) {
			ERROR_FN(Locale::get("ERROR_FONT_FILE"),text->_font.c_str());
        }else {
			if (!tempFont->FaceSize(text->_height)) {
				ERROR_FN(Locale::get("ERROR_FONT_HEIGHT"),text->_height);
            }
        }
		_fonts.insert(std::make_pair(fontName, tempFont));

	}
	glPushAttrib(GL_COLOR_BUFFER_BIT);
	glColor3f(text->_color.x,text->_color.y,text->_color.z);
	glPushMatrix();
		glLoadIdentity();
		_modelViewMatrix.push(mat4<GLfloat>()); //glPushMatrix(); glLoadIdentity();

			GLCheck(glRasterPos2f(text->getPosition().x,_cachedResolution.height - text->getPosition().y));
			_fonts[fontName]->Render(text->_text.c_str());

	glPopMatrix();
	_modelViewMatrix.pop();
	glColor3i(1,1,1);
	glPopAttrib();
}

void GL_API::toggleDepthMapRendering(bool state) {
	if(state){
		if(!_depthMapRendering){
			GLCheck(glShadeModel(GL_FLAT));
			_depthMapRendering = true;
		}
	}else{
		if(_depthMapRendering){
			GLCheck(glShadeModel(GL_SMOOTH));
			_depthMapRendering = false;
		}
	}
}

void GL_API::updateStateInternal(RenderStateBlock* block, bool force){
   glRenderStateBlock* glBlock = static_cast<glRenderStateBlock*>(block);
   assert(glBlock != NULL);
   glRenderStateBlock* glCurrent = static_cast<glRenderStateBlock*>(GFX_DEVICE._currentStateBlock);
   if (force){
      glCurrent = NULL;
   }
   glBlock->activate(glCurrent);
   _currentGLRenderStateBlock = glBlock;
}

/// Applies matrix transformations to the vertices that are about to be drawn
/// If now shader is specified, fixed function multiplication are used for compatibility
void GL_API::setObjectState(Transform* const transform, bool force, ShaderProgram* const shader){
	if(transform){

		glPushMatrix();
		_modelViewMatrix.push(_modelViewMatrix.top());
		if(force){
			glLoadIdentity();
			_modelViewMatrix.top().identity();
		}
		mat4<F32> transformMatrix = transform->getMatrix() * transform->getParentMatrix();
		if(shader && shader->isBound()){
			
			shader->Uniform("transformMatrix",transformMatrix);
		}//else{
			GLCheck(glMultMatrixf(transformMatrix));

			_modelViewMatrix.top() *= transform->getMatrix();
			_modelViewMatrix.top() *= transform->getParentMatrix();
		//}
	}
}

void GL_API::releaseObjectState(Transform* const transform, ShaderProgram* const shader){
	if(transform){
		glPopMatrix();
		_modelViewMatrix.pop();
	}
}
 
void GL_API::renderInViewport(const vec4<GLfloat>& rect, boost::function0<void> callback){
	
	GLCheck(glPushAttrib(GL_VIEWPORT_BIT | GL_DEPTH_BUFFER_BIT));
	GLCheck(glDisable(GL_DEPTH_TEST));
    GLCheck(glViewport(rect.x, rect.y, rect.z, rect.w));
	callback();
	GLCheck(glPopAttrib());

}

IMPrimitive* GL_API::getOrCreateIMPrimitive(){
	NS_GLIM::GLIM_BATCH* IMInterface = NULL;
	IMPrimitive* tempPriv = NULL;
	///Find a zombified primitive
	vectorImpl<IMPrimitive* >::iterator it;
	it = std::find_if(_glimInterfaces.begin(),_glimInterfaces.end(),IMPrimitiveValidation::isValid);
	if(it != _glimInterfaces.end()){///If we have one ...
		tempPriv = *it;
		///... resurrect it
		tempPriv->_zombieCounter = 0;
		IMInterface->Clear();
	}else{///If we do not have a valid zombie, add a new element
		tempPriv = New IMPrimitive();
		assert(tempPriv->_imInterface != NULL);
		_glimInterfaces.push_back(tempPriv);
	}
	IMInterface = tempPriv->_imInterface;
	assert(IMInterface != NULL);
	assert(tempPriv != NULL);
	tempPriv->_inUse = true;
	tempPriv->_setupStates.clear();
	tempPriv->_resetStates.clear();
	return tempPriv;
}

void GL_API::drawBox3D(const vec3<GLfloat>& min,const vec3<GLfloat>& max, const mat4<GLfloat>& globalOffset){

	IMPrimitive* priv = getOrCreateIMPrimitive();
	NS_GLIM::GLIM_BATCH* IMInterface = priv->_imInterface;

	priv->_hasLines = true;
	priv->_lineWidth = 2.0f;

	IMInterface->BeginBatch();

	IMInterface->Attribute4ub("inColorData", 0, 0, 255,255);
	IMInterface->Begin(NS_GLIM::GLIM_LINE_LOOP);
		IMInterface->Vertex( min.x, min.y, min.z );
		IMInterface->Vertex( max.x, min.y, min.z );
		IMInterface->Vertex( max.x, min.y, max.z );
		IMInterface->Vertex( min.x, min.y, max.z );
	IMInterface->End();

	IMInterface->Begin(NS_GLIM::GLIM_LINE_LOOP);
		IMInterface->Vertex( min.x, max.y, min.z );
		IMInterface->Vertex( max.x, max.y, min.z );
		IMInterface->Vertex( max.x, max.y, max.z );
		IMInterface->Vertex( min.x, max.y, max.z );
	IMInterface->End();

	IMInterface->Begin(NS_GLIM::GLIM_LINES);
		IMInterface->Vertex( min.x, min.y, min.z );
		IMInterface->Vertex( min.x, max.y, min.z );
		IMInterface->Vertex( max.x, min.y, min.z );
		IMInterface->Vertex( max.x, max.y, min.z );
		IMInterface->Vertex( max.x, min.y, max.z );
		IMInterface->Vertex( max.x, max.y, max.z );
		IMInterface->Vertex( min.x, min.y, max.z );
		IMInterface->Vertex( min.x, max.y, max.z );
	IMInterface->End();
	IMInterface->EndBatch();
}

void GL_API::setupSkeletonState(OffsetMatrix mat){
	glPushMatrix();
	//glLoadIdentity();
	_modelViewMatrix.push(_modelViewMatrix.top());
	mat4<GLfloat> m(mat.mat);
	_modelViewMatrix.top() *= m;
	GLCheck(glMultMatrixf(mat.mat));
	GLCheck(glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT));
    GLCheck(glDisable(GL_DEPTH_TEST));  
}

void GL_API::releaseSkeletonState(){
	GLCheck(glPopAttrib());
	glPopMatrix();
	_modelViewMatrix.pop();
}

void GL_API::drawLines(const vectorImpl<vec3<GLfloat> >& pointsA,const vectorImpl<vec3<GLfloat> >& pointsB,const vectorImpl<vec4<GLfloat> >& colors, const mat4<GLfloat>& globalOffset){
	/// We need a perfect pair of point A's to point B's
	if(pointsA.size() != pointsB.size()) return;
	/// We need a color for each line, even if it is the same one
	if(pointsA.size() != colors.size()) return;
	

	IMPrimitive* priv = getOrCreateIMPrimitive();
	NS_GLIM::GLIM_BATCH* IMInterface = priv->_imInterface;
	OffsetMatrix offset;
	std::copy(globalOffset.mat, globalOffset.mat+16, offset.mat);
	priv->_hasLines = true;
	priv->_lineWidth = 5.0f;
	priv->_setupStates = boost::bind(&GL_API::setupSkeletonState, this, offset);
	priv->_resetStates = boost::bind(&GL_API::releaseSkeletonState, this);
	IMInterface->BeginBatch();
	IMInterface->Attribute4f("inColorData", 1.0f,0.0f,0.0f,1.0f);
    IMInterface->Begin(NS_GLIM::GLIM_LINES);
 
	for(GLushort i = 0; i < pointsA.size(); i++){
		IMInterface->Attribute4f("inColorData", colors[i].r, colors[i].g, colors[i].b,colors[i].a);
		IMInterface->Vertex( pointsA[i].x,pointsA[i].y,pointsA[i].z);
		IMInterface->Vertex( pointsB[i].x,pointsB[i].y,pointsB[i].z);
	}
    IMInterface->End();
	IMInterface->EndBatch();
	
}

void GL_API::render3DText(Text3D* const text){
	assert(text != NULL);

	const char* fontName = text->getFont().c_str();
	FontCache::iterator itr = _3DFonts.find(fontName);
	if(itr == _3DFonts.end()){
		std::string fontPath = ParamHandler::getInstance().getParam<std::string>("assetsLocation") + "/";
		fontPath += "GUI/";
		fontPath += "fonts/";
		fontPath += fontName;
		FTFont* tempFont = New FTGLPolygonFont(fontPath.c_str());
		if (!tempFont) {
			ERROR_FN(Locale::get("ERROR_FONT_FILE"),fontName);
        }else {
			if (!tempFont->FaceSize(text->getHeight())) {
				ERROR_FN(Locale::get("ERROR_FONT_HEIGHT"),text->getHeight());
            }
        }
		_3DFonts.insert(std::make_pair(fontName, tempFont));

	}
	GLCheck(glPushAttrib(GL_ENABLE_BIT));
	GLCheck(glLineWidth(text->getWidth()));
	
	_3DFonts[fontName]->Render(text->getText().c_str());


	GLCheck(glPopAttrib());
}

void GL_API::renderModel(Object3D* const model){
    if(model->getType() == Object3D::TEXT_3D){
		render3DText(dynamic_cast<Text3D*>(model));
		return;
	}

    if(model->getType() ==	Object3D::OBJECT_3D_PLACEHOLDER){
		ERROR_FN(Locale::get("ERROR_GL_INVALID_OBJECT_TYPE"),model->getName().c_str());
		return;
	}		
	
	VertexBufferObject* vbo = model->getGeometryVBO();
	assert(vbo != NULL);
    //Choose optimal VAO/VBO combo
    vbo->setDepthPass(GFX_DEVICE.isCurrentRenderStage(DEPTH_STAGE));
    //Render selected combo
	vbo->Draw(model->getCurrentLOD());
}

void GL_API::renderModel(VertexBufferObject* const vbo, GFXDataFormat f, U32 count, const void* first_element){
    assert(vbo != NULL);
    vbo->setDepthPass(GFX_DEVICE.isCurrentRenderStage(DEPTH_STAGE));
    vbo->Draw(f,count,first_element);
}

void GL_API::toggle2D(bool state){
	if(!_state2DRendering){
		RenderStateBlockDescriptor state2DRenderingDesc;
		state2DRenderingDesc.setCullMode(CULL_MODE_NONE);
		state2DRenderingDesc._fixedLighting = false;
		_state2DRendering = GFX_DEVICE.createStateBlock(state2DRenderingDesc);
	}

	if(state){ //2D
        if(_2DRendering) return;
        _2DRendering = true;
		_previousStateBlock = SET_STATE_BLOCK(_state2DRendering);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix(); //1
		glLoadIdentity();
		//glPushMatrix();glLoadIdentity();glOrtho();
		_projectionMatrix.push(Divide::GL::_ortho(0,_cachedResolution.width,0,_cachedResolution.height,-1,1)); 

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix(); //2
		glLoadIdentity();
		_modelViewMatrix.push(mat4<GLfloat>()); //glPushMatrix();glLoadIdentity();
	}else{ //3D
        if(!_2DRendering) return;
		_modelViewMatrix.pop(); //glPopMatrix();
		glPopMatrix(); //2 
		_projectionMatrix.pop(); //glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix(); //1
		SET_STATE_BLOCK(_previousStateBlock);
        _2DRendering = false;
	}
}

void GL_API::setAmbientLight(const vec4<GLfloat>& light){
	GLCheck(glLightModelfv(GL_LIGHT_MODEL_AMBIENT, light));
}

///Update OpenGL light state
void GL_API::setLight(Light* const light){
	assert(light != NULL);
	GLubyte slot = light->getSlot();
	if(!light->getEnabled()){
	   GLCheck(glDisable(GL_LIGHT0 + slot));
      return;
   }
	if(slot >= 8){
		///We only have 8 slots, so replace
		slot = slot%8;
	}

	LightType type = light->getLightType();
	vec4<GLfloat> position(light->getPosition());
	position.w = (type == LIGHT_TYPE_DIRECTIONAL) ? 0.0f : 1.0f;
	GLCheck(glLightfv(GL_LIGHT0+slot, GL_POSITION, position));

	GLCheck(glLightfv(GL_LIGHT0+slot, GL_AMBIENT,  light->getVProperty(LIGHT_PROPERTY_AMBIENT)));
	GLCheck(glLightfv(GL_LIGHT0+slot, GL_DIFFUSE,  light->getVProperty(LIGHT_PROPERTY_DIFFUSE)));
	GLCheck(glLightfv(GL_LIGHT0+slot, GL_SPECULAR, light->getVProperty(LIGHT_PROPERTY_SPECULAR)));
	if(type == LIGHT_TYPE_SPOT){
		GLCheck(glLightfv(GL_LIGHT0+slot, GL_SPOT_DIRECTION, light->getDirection()));
		GLCheck(glLightf(GL_LIGHT0+slot, GL_SPOT_EXPONENT,light->getFProperty(LIGHT_PROPERTY_SPOT_EXPONENT)));
		GLCheck(glLightf(GL_LIGHT0+slot, GL_SPOT_CUTOFF, light->getFProperty(LIGHT_PROPERTY_SPOT_CUTOFF)));
	}
	if(type != LIGHT_TYPE_DIRECTIONAL){
		GLCheck(glLightf(GL_LIGHT0+slot, GL_CONSTANT_ATTENUATION,light->getFProperty(LIGHT_PROPERTY_CONST_ATT)));
		GLCheck(glLightf(GL_LIGHT0+slot, GL_LINEAR_ATTENUATION,light->getFProperty(LIGHT_PROPERTY_LIN_ATT)));
		GLCheck(glLightf(GL_LIGHT0+slot, GL_QUADRATIC_ATTENUATION,light->getFProperty(LIGHT_PROPERTY_QUAD_ATT)));
	}
	GLCheck(glEnable(GL_LIGHT0+slot));
}


//Setting _LookAt here for camera's or shadow projection
// -set the current matrix to GL_MODELVIEW
// -reset it to identity
// -invert the scene if needed by using, the now deprecated, I know, glScalef
void GL_API::lookAt(const vec3<GLfloat>& eye,const vec3<GLfloat>& center,const vec3<GLfloat>& up, bool invertx, bool inverty){
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	_modelViewMatrix.top().identity();

	if(invertx){
		GLCheck(glScalef(-1.0f,1.0f,1.0f));
		_modelViewMatrix.top().scale(-1.0f,1.0f,1.0f);
	}
	_modelViewMatrix.top() *= Divide::GL::_LookAt(	eye.x,		eye.y,		eye.z,
												    center.x,	center.y,	center.z,
													up.x,		up.y,		up.z	);
}

void GL_API::lockProjection(){
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	_projectionMatrix.push(_projectionMatrix.top()); //glPushMatrix();
}

void GL_API::releaseProjection(){
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	_projectionMatrix.pop(); //glPopMatrix();
}

void GL_API::lockModelView(){
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	_modelViewMatrix.push(_modelViewMatrix.top()); //glPushMatrix();
}

void GL_API::releaseModelView(){
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	_modelViewMatrix.pop(); //glPopMatrix();
}

//Setting ortho projection:
// -sets the current matrix to GL_PROJECTION
// -resets it to identity
// -sets an ortho perspective based on the input rect and limits
// -and sets the matrix mode back to GL_MODELVIEW
void GL_API::setOrthoProjection(const vec4<GLfloat>& rect, const vec2<GLfloat>& planes){
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//glLoadIdentity();glOrtho();
	_projectionMatrix.top() = Divide::GL::_ortho(rect.x, rect.y, rect.z, rect.w, planes.x, planes.y);
	glMatrixMode(GL_MODELVIEW);
}

//Setting perspective projection:
// -sets the current matrix to GL_PRJECTION
// -resets it to identity
// -sets a perspective projection based on the input FoV, aspect ration and limits
// -and sets the matrix mode back to GL_MODELVIEW
void GL_API::setPerspectiveProjection(GLfloat FoV,GLfloat aspectRatio, const vec2<GLfloat>& planes){
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	//glPushMatrix(); glLoadIdentity(); glOrtho();
	_projectionMatrix.push(Divide::GL::_perspective(FoV, aspectRatio, planes.x, planes.y));
	glMatrixMode(GL_MODELVIEW);
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
	ImageTools::SaveSeries(filename,w,h,32,imageData);
}

// this function builds a projection matrix for rendering from the shadow's POV.
// First, it computes the appropriate z-range and sets an orthogonal projection.
// Then, it translates and scales it, so that it exactly captures the bounding box
// of the current frustum slice
GLfloat GL_API::applyCropMatrix(frustum &f,SceneGraph* sceneGraph){
	GLfloat shad_modelview[16];
	GLfloat shad_proj[16];
	GLfloat maxX = -1000.0f;
    GLfloat maxY = -1000.0f;
	GLfloat maxZ;
    GLfloat minX =  1000.0f;
    GLfloat minY =  1000.0f;
	GLfloat minZ;

	mat4<GLfloat> nv_mvp;
	vec4<GLfloat> transf;	
	
	// find the z-range of the current frustum as seen from the light
	// in order to increase precision
	GLCheck(glGetFloatv(GL_MODELVIEW_MATRIX, nv_mvp.mat));
	
	// note that only the z-component is need and thus
	// the multiplication can be simplified
	// transf.z = shad_modelview[2] * f.point[0].x + shad_modelview[6] * f.point[0].y + shad_modelview[10] * f.point[0].z + shad_modelview[14];
	transf = nv_mvp*vec4<GLfloat>(f.point[0], 1.0f);
	minZ = transf.z;
	maxZ = transf.z;
	for(GLubyte i=1; i<8; i++){
		transf = nv_mvp*vec4<GLfloat>(f.point[i], 1.0f);
		if(transf.z > maxZ) maxZ = transf.z;
		if(transf.z < minZ) minZ = transf.z;
	}
	// make sure all relevant shadow casters are included
	// note that these here are dummy objects at the edges of our scene
		//Unload every sub node recursively

	for_each(BoundingBox& b, sceneGraph->getBBoxes()){
		transf = nv_mvp*vec4<GLfloat>(b.getCenter(), 1.0f);
		if(transf.z + b.getHalfExtent().z > maxZ) maxZ = transf.z + b.getHalfExtent().z;
	//	if(transf.z - b.radius < minZ) minZ = transf.z - b.radius;
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// set the projection matrix with the new z-bounds
	// note the inversion because the light looks at the neg. z axis
	// _perspective(LIGHT_FOV, 1.0, maxZ, minZ); // for point lights
	 //glLoadIdentity(); //glMultMatrixf(...);
	_projectionMatrix.top() = Divide::GL::_ortho(-1.0, 1.0, -1.0, 1.0, -maxZ, -minZ);

	GLCheck(glGetFloatv(GL_PROJECTION_MATRIX, shad_proj));
	glPushMatrix();
	GLCheck(glMultMatrixf(shad_modelview));
	_projectionMatrix.push(_projectionMatrix.top()); //glPushMatrix();
	mat4<GLfloat> m(shad_modelview);
	_projectionMatrix.top() *= m; //glMultMatrixf(...);

	GLCheck(glGetFloatv(GL_PROJECTION_MATRIX, nv_mvp.mat));
	glPopMatrix();
	_projectionMatrix.pop();//glPopMatrix();

	// find the extends of the frustum slice as projected in light's homogeneous coordinates
	for(GLubyte i=0; i<8; i++){
		transf = nv_mvp*vec4<GLfloat>(f.point[i], 1.0f);

		transf.x /= transf.w;
		transf.y /= transf.w;

		if(transf.x > maxX) maxX = transf.x;
		if(transf.x < minX) minX = transf.x;
		if(transf.y > maxY) maxY = transf.y;
		if(transf.y < minY) minY = transf.y;
	}

	GLfloat scaleX = 2.0f/(maxX - minX);
	GLfloat scaleY = 2.0f/(maxY - minY);
	GLfloat offsetX = -0.5f*(maxX + minX)*scaleX;
	GLfloat offsetY = -0.5f*(maxY + minY)*scaleY;

	// apply a crop matrix to modify the projection matrix we got from glOrtho.
	nv_mvp.identity();
	nv_mvp.element(0,0) = scaleX;
	nv_mvp.element(1,1) = scaleY;
	nv_mvp.element(0,3) = offsetX;
	nv_mvp.element(1,3) = offsetY;
	nv_mvp.transpose();
	GLCheck(glLoadMatrixf(nv_mvp.mat));
	GLCheck(glMultMatrixf(shad_proj));
	mat4<GLfloat> m2(nv_mvp);
	mat4<GLfloat> m3(shad_proj);
	_projectionMatrix.top() = m2; //glLoadMatrixf(...);
	_projectionMatrix.top() *= m3; //glMultMatrixf(...);
	return minZ;
}

RenderStateBlock* GL_API::newRenderStateBlock(const RenderStateBlockDescriptor& descriptor){
	return New glRenderStateBlock(descriptor);
}


FrameBufferObject* GL_API::newFBO(FBOType type)  {
	switch(type){
		case FBO_2D_DEFERRED:
			return New glDeferredBufferObject(); 
		case FBO_2D_DEPTH:
			return New glDepthBufferObject(); 
		case FBO_2D_ARRAY_DEPTH:
			return New glDepthArrayBufferObject();
		case FBO_CUBE_COLOR:
			return New glTextureBufferObject(true);
		case FBO_CUBE_DEPTH:
			return New glTextureBufferObject(true,true);
		case FBO_CUBE_COLOR_ARRAY:
			return New glTextureArrayBufferObject(true);
		case FBO_CUBE_DEPTH_ARRAY:
			return New glTextureArrayBufferObject(true,true);
		case FBO_2D_ARRAY_COLOR:
			return New glTextureArrayBufferObject();
		case FBO_2D_COLOR_MS:{
			if(_msaaSamples > 1 && _useMSAA)
				return New glMSTextureBufferObject(); ///<No MS cube support yet
			else
				return New glTextureBufferObject();
		}
		default:
		case FBO_2D_COLOR:
			return New glTextureBufferObject(); 
	}
}

VertexBufferObject* GL_API::newVBO(PrimitiveType type) {
	return New glVertexArrayObject(type); 
}

PixelBufferObject* GL_API::newPBO(PBOType type) {
	return New glPixelBufferObject(type); 
}

void GL_API::setActiveTextureUnit(U32 unit){
    setActiveTextureUnitInternal(unit);
	GLCheck(glActiveTexture(GL_TEXTURE0 + unit));
}

void GL_API::setClientActiveTextureUnit(U32 unit){
    setClientActiveTextureUnitInternal(unit);
    GLCheck(glClientActiveTexture(GL_TEXTURE0 + unit));
}

void GL_API::setActiveVBO(U32 id){
    setActiveVBOIdInternal(id);
    GLCheck(glBindBuffer(GL_ARRAY_BUFFER, id));
}

void GL_API::setActiveIBO(U32 id){
    GLCheck(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id));
}

void GL_API::setActiveVAO(U32 id){
    setActiveVAOIdInternal(id);
    GLCheck(glBindVertexArray(id));
}