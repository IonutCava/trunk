/*“Copyright 2009-2011 DIVIDE-Studio”*/
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

#ifndef _SCENE_NODE_H_
#define _SCENE_NODE_H_

#include "Utility/Headers/Transform.h"
#include "Utility/Headers/BoundingBox.h"
#include "Utility/Headers/BaseClasses.h"
#include "Geometry/Material/Material.h"
#include "Utility/Headers/Console.h"

class SceneNode : public Resource {
public:
	SceneNode() : Resource(),
				 _transform(NULL),
				 _material(NULL),
				 _renderState(true),
				_noDefaultMaterial(false){}
	SceneNode(std::string name) : Resource(name),
								  _transform(NULL),
								  _material(NULL),
								  _renderState(true),
								  _noDefaultMaterial(false){}
	SceneNode(const SceneNode& old);
	virtual ~SceneNode() {}
	/*Rendering/Processing*/
	virtual void render() = 0; //Sounds are played, geometry is displayed etc.
			void setRenderState(bool state) {_renderState = state;}
			bool getRenderState() {return _renderState;}
	/*//Rendering/Processing*/	

	virtual	bool        unload();
	inline  const		BoundingBox&    	getOriginalBoundingBox()		  {return _originalBB;}
	virtual inline		BoundingBox&		getBoundingBox();
			void		setTransform(Transform* t);
		    Transform*  const getTransform();
			void		setOriginalBoundingBox(BoundingBox& originalBB){_originalBB = originalBB;}
			void		setMaterial(Material* m);
			void		clearMaterials();
		    Material*	getMaterial();

	virtual	bool    computeBoundingBox() {return true;}
	virtual void    drawBBox();
	void    useDefaultMaterial(bool state) {_noDefaultMaterial = !state;}
private:
	//_originalBB is a copy of the initialy calculate BB for transformation
	//it should be copied in every computeBoungingBox call;
	BoundingBox _originalBB; 
	Material*	_material;				   
	Transform*	_transform;
	bool		_renderState,_noDefaultMaterial;

protected:
	BoundingBox	 _boundingBox;
};

#endif