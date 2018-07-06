/*
   Copyright (c) 2013 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.
   
   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef REFLECTOR_H_
#define REFLECTOR_H_

#include "Managers/Headers/FrameListenerManager.h"

enum ReflectorType{
	TYPE_MIRROR,
	TYPE_WATER_SURFACE,
	TYPE_PORTAL,
	TYPE_MATERIAL,
	TYPE_REFLECTOR_PLACEHOLDER
};

class Quad3D;
class ShaderProgram;
class FrameBufferObject;
/// Virtual interface for reflective surfaces
class Reflector : public FrameListener{
public:
	Reflector(ReflectorType type,const vec2<U16>& resolution);
	virtual ~Reflector();

	///This function should be unique to every reflector. Portals may need special effects, mirrors some special lighting, etc
	virtual void updateReflection() = 0;

	inline FrameBufferObject* getReflectionFBO(){return _reflectedTexture;}

	///Rendering callback is that function in the scene used to render the reflection
	///The SceneGraph is not the default rendering class for reflections as some elements (i.e. Sky) are not part of it
	///As the sky and other elements should be included in the reflection, we should set a custom callback
	///Another example would be the player model. It is not rendered when in First Person, but it should show up in reflections
	inline void setRenderCallback(boost::function0<void> callback) {_renderCallback = callback;}

	///This is inherited from FrameListener and is used to queue up reflection on every frame start
	bool framePreRenderEnded(const FrameEvent& evt);
    void togglePreviewReflection() {_previewReflection = !_previewReflection;}
    ///Draw a small quad with our reflection buffer as a texture
    void previewReflection();

protected:

	ReflectorType _type;
	/// How often do we need to update this instance's reflection?
	U32 _updateInterval;
	/// What is our current tick count
	F32 _updateTimer;
	/// What resolution should the generated texture have?
	vec2<U16> _resolution;
	/// Use this to force current reflector to draw itself in reflection
	bool _excludeSelfReflection;
    bool _previewReflection;
private:
	/// Create FBO
	bool  build();

protected:
	boost::function0<void> _renderCallback;
	FrameBufferObject* _reflectedTexture;
    Quad3D* _renderQuad;
	ShaderProgram* _previewReflectionShader;
	/// used for render exclusion. Do not render self in own reflection
	bool _updateSelf;
	/// Have we initialized our FBO?
	bool _createdFBO;
};

#endif