/*“Copyright 2009-2012 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef REFLECTOR_H_
#define REFLECTOR_H_

#include "Managers/Headers/FrameListenerManager.h"

enum REFLECTOR_TYPE{
	TYPE_MIRROR,
	TYPE_WATER_SURFACE,
	TYPE_PORTAL,
	TYPE_MATERIAL,
	TYPE_REFLECTOR_PLACEHOLDER
};

class FrameBufferObject;
/// Virtual interface for reflective surfaces
class Reflector : public FrameListener{
public:
	Reflector(REFLECTOR_TYPE type,const vec2<U16>& resolution);
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

private:

	REFLECTOR_TYPE _type;
	/// How often do we need to update this instance's reflection?
	U32 _updateInterval;
	/// What is our current tick count
	F32 _updateTimer;
	/// What resolution should the generated texture have?
	vec2<U16> _resolution;
	/// Use this to force current reflector to draw itself in reflection
	bool _excludeSelfReflection;

private:
	/// Create FBO
	bool  build();

protected:
	boost::function0<void> _renderCallback;
	FrameBufferObject* _reflectedTexture;
	/// used for render exclusion. Do not render self in own reflection
	bool _updateSelf;
	/// Have we initialized our FBO?
	bool _createdFBO;

};

#endif