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

#ifndef _SCENE_NODE_H_
#define _SCENE_NODE_H_

#include "SceneNodeRenderState.h"
#include "Core/Resources/Headers/Resource.h"
#include "Core/Math/Headers/Transform.h"
#include "Geometry/Material/Headers/Material.h"
#include "Dynamics/Physics/Headers/PhysicsAsset.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingSphere.h"

enum SceneNodeType{
	TYPE_ROOT             = toBit(1),
	TYPE_OBJECT3D         = toBit(2),
	TYPE_TERRAIN          = toBit(3),
	TYPE_WATER            = toBit(4),
	TYPE_LIGHT            = toBit(5),
	TYPE_TRIGGER          = toBit(6),
	TYPE_PARTICLE_EMITTER = toBit(7),
	TYPE_SKY              = toBit(8),
	///Place types above
	TYPE_PLACEHOLDER      = toBit(10)
};

class Scene;
enum  RenderStage;
class SceneNode : public Resource {

	friend class SceneGraphNode;
	friend class RenderQueue;

public:
	SceneNode(SceneNodeType type);
	SceneNode(std::string name, SceneNodeType type);
	virtual ~SceneNode();

	/*Rendering/Processing*/
	virtual void render(SceneGraphNode* const sgn) = 0; //Sounds are played, geometry is displayed etc.

	virtual	bool getDrawState()  const {return _renderState.getDrawState();} 
	/// Some scenenodes may need special case handling. I.E. water shouldn't render itself in REFLECTION_STAGE
	virtual	bool getDrawState(RenderStage currentStage)  const {return _renderState.getDrawState(currentStage); }
	/*//Rendering/Processing*/	

	virtual	bool			unload();
	virtual bool	        isInView(bool distanceCheck,BoundingBox& boundingBox,const BoundingSphere& sphere);
    virtual	void			setMaterial(Material* m);
			void			clearMaterials();
		    Material*		getMaterial();

	/* Normal material */
	virtual	void            prepareMaterial(SceneGraphNode* const sgn);
	virtual	void            releaseMaterial();
	///setup any derived class special constants, such as animation information or reflection stuff)
	virtual void            setSpecialShaderConstants(ShaderProgram* const shader);
	/* Depth map material */
	virtual	void            prepareDepthMaterial(SceneGraphNode* const sgn);
	virtual	void            releaseDepthMaterial();

	/// Every SceneNode computes a bounding box in it's own way. 
	virtual	bool    computeBoundingBox(SceneGraphNode* const sgn);
	/// Special BB transforms may be required at a certain frame
	virtual void    updateBBatCurrentFrame(SceneGraphNode* const sgn);
	virtual void    updateTransform(SceneGraphNode* const sgn) {};
	/// Perform any pre-draw operations (this is after sort and transform updates)
	virtual void    onDraw();
	/// Perform any post=draw operations (this is after releasing object and shadow states)
	virtual void    postDraw() {/*Nothing yet*/}
	/// Perform any last minute operations before the frame drawing ends (this is after shader and shodawmap unbindng)
	virtual void    preFrameDrawEnd(SceneGraphNode* const sgn);
	virtual void    drawBoundingBox(SceneGraphNode* const sgn);
	virtual void    postLoad(SceneGraphNode* const sgn) = 0; //Post insertion calls (Use this to setup child objects during creation)
	/// Called from SceneGraph "sceneUpdate"
	virtual void sceneUpdate(U32 sceneTime) {};

	inline 	void	setSelected(bool state)  {_selected = state;}
	inline 	bool    isSelected()	const	 {return _selected;}

	inline SceneNodeType getType()					 {return _type;}
	inline void          setType(SceneNodeType type) {_type = type;}

	inline SceneNodeRenderState& getSceneNodeRenderState() {return _renderState;}

	inline void incLODcount() {_LODcount++;}
	inline void decLODcount() {_LODcount--;}
	inline U8   getLODcount() {return _LODcount;}
	inline U8   getCurrentLOD() {return (_lodLevel < (_LODcount-1) ? _lodLevel : (_LODcount-1));}
    
protected:

	SceneNodeRenderState  _renderState;
	///Attach a physics asset to the node to make it physics enabled
	PhysicsAsset*         _physicsAsset;
	///LOD level is updated at every visibility check (SceneNode::isInView(...));
	U8                    _lodLevel; ///<Relative to camera distance
	U8                    _LODcount; ///<Maximum available LOD levels

private:
    //mutable SharedLock _materialLock;
	Material*	  _material;				   
	bool          _selected;
	SceneNodeType _type;
};

#endif