/*“Copyright 2009-2013 DIVIDE-Studio”*/
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

#ifndef _RENDERER_H_
#define _RENDERER_H_
#include <boost/function.hpp>
class SceneRenderState;

enum RendererType{
	RENDERER_FORWARD = 0,
	RENDERER_DEFERRED_SHADING,
	RENDERER_DEFERRED_LIGHTING,
	RENDERER_PLACEHOLDER
};
///An abstract renderer used to switch between different rendering tehniques: Forward, Deferred Shading or Deferred Lighting
class Renderer {
public:
	Renderer(RendererType type) : _type(type) {}
	virtual ~Renderer() {}
	virtual void render(boost::function0<void> renderCallback, SceneRenderState* const sceneRenderState) = 0;
	virtual void toggleDebugView() = 0;
	inline RendererType getType() {return _type;}
	inline std::string  getTypeToString(){
		switch(_type){
			default:
			case RENDERER_PLACEHOLDER:
				return "Unknown Renderer Type";
			case RENDERER_FORWARD:
				return "Forward Renderer";
			case RENDERER_DEFERRED_SHADING:
				return "Deferred Shading Renderer";
			case RENDERER_DEFERRED_LIGHTING:
				return "Deferred Lighting Renderer";
		}
	}
protected:
	RendererType _type;
};

#endif