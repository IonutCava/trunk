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

#ifndef _SCENE_NODE_H_
#define _SCENE_NODE_H_

#include "Core/Headers/BaseClasses.h"
#include "Core/Math/Headers/Transform.h"
#include "Utility/Headers/BoundingBox.h"
#include "Geometry/Material/Headers/Material.h"

enum SCENE_NODE_TYPE{
	TYPE_ROOT             = toBit(1),
	TYPE_OBJECT3D         = toBit(2),
	TYPE_TERRAIN          = toBit(3),
	TYPE_WATER            = toBit(4),
	TYPE_LIGHT            = toBit(5),
	TYPE_TRIGGER          = toBit(6),
	TYPE_PARTICLE_EMITTER = toBit(7),

	///Place types above
	TYPE_PLACEHOLDER      = toBit(10)
};

class Scene;
enum  RENDER_STAGE;
class SceneNode : public Resource {
	friend class SceneGraphNode;
	friend class RenderQueue;
public:
	SceneNode(SCENE_NODE_TYPE type);
	SceneNode(std::string name, SCENE_NODE_TYPE type);
	virtual ~SceneNode();

	/*Rendering/Processing*/
	virtual void render(SceneGraphNode* const sgn) = 0; //Sounds are played, geometry is displayed etc.
			void setDrawState(bool state) {_drawState = state;}
	virtual	bool getDrawState()  const {return _drawState;} 
			void addToDrawExclusionMask(U8 stageMask);
			void removeFromDrawExclusionMask(U8 stageMask);
	/// Some scenenodes may need special case handling. I.E. water shouldn't render itself in REFLECTION_STAGE
	virtual	bool getDrawState(RENDER_STAGE currentStage)  const;
	/*//Rendering/Processing*/	

	virtual	bool			unload();

	virtual bool	        isInView(bool distanceCheck,BoundingBox& boundingBox);
    virtual	void			setMaterial(Material* m);
			void			clearMaterials();
		    Material*		getMaterial();

	/* Normal material */
	virtual	void            prepareMaterial(SceneGraphNode* const sgn);
	virtual	void            releaseMaterial();
	/* Depth map material */
	virtual	void            prepareShadowMaterial(SceneGraphNode* const sgn);
	virtual	void            releaseShadowMaterial();

	/// Every SceneNode computes a bounding box in it's own way. 
	virtual	bool    computeBoundingBox(SceneGraphNode* const sgn);
	virtual void    updateTransform(SceneGraphNode* const sgn);
	virtual void    onDraw();
	virtual void    postDraw();
	virtual void    drawBoundingBox(SceneGraphNode* const sgn);
	virtual void    postLoad(SceneGraphNode* const sgn) = 0; //Post insertion calls (Use this to setup child objects during creation)
	void    useDefaultMaterial(bool state) {_noDefaultMaterial = !state;}
	/// Called from SceneGraph "sceneUpdate"
	virtual void sceneUpdate(D32 sceneTime);

	inline	void	setSelected(bool state)  {_selected = state;}
	inline	bool    isSelected()	const	 {return _selected;}
	virtual void    createCopy();
	virtual void    removeCopy();

	SCENE_NODE_TYPE getType()					  {return _type;}
	void            setType(SCENE_NODE_TYPE type) {_type = type;}

protected:
	U8          _exclusionMask;

private:
	
	Material*	_material;				   
	RenderStateBlock* _shadowStateBlock;

	bool		_drawState;
	bool        _noDefaultMaterial;
	bool        _selected;
	SCENE_NODE_TYPE _type;
};

#endif