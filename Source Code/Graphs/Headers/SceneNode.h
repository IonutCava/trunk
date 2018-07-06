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

#include "Core/Headers/BaseClasses.h"
#include "Core/Math/Headers/Transform.h"
#include "Utility/Headers/BoundingBox.h"
#include "Geometry/Material/Headers/Material.h"

class Scene;
enum  RENDER_STAGE;
class SceneNode : public Resource {
	friend class SceneGraphNode;
	friend class RenderQueue;
public:
	SceneNode();
	SceneNode(std::string name);

	virtual ~SceneNode() {}
	/*Rendering/Processing*/
	virtual void render(SceneGraphNode* const node) = 0; //Sounds are played, geometry is displayed etc.
			void setRenderState(bool state) {_renderState = state;}
			bool getRenderState()  const {return _renderState;} 
			void addToRenderExclusionMask(U8 stageMask);
			void removeFromRenderExclusionMask(U8 stageMask);
			bool getRenderState(RENDER_STAGE currentStage)  const;
	/*//Rendering/Processing*/	

	virtual	bool			unload();

	virtual bool	        isInView(bool distanceCheck,BoundingBox& boundingBox);
    virtual	void			setMaterial(Material* m);
			void			clearMaterials();
		    Material*		getMaterial();
	virtual	void            prepareMaterial(SceneGraphNode* const sgn);
	virtual	void            releaseMaterial();
	virtual	bool    computeBoundingBox(SceneGraphNode* const node);
	virtual void    onDraw();
	virtual void    postLoad(SceneGraphNode* const node) = 0; //Post insertion calls (Use this to setup child objects during creation)
	void    useDefaultMaterial(bool state) {_noDefaultMaterial = !state;}
	

	inline	void	setSelected(bool state)  {_selected = state;}
	inline	bool    isSelected()	const	 {return _selected;}
	virtual void    createCopy();
	virtual void    removeCopy();
	virtual void    changeSortKey(I32 key) {_sortKey = key;}

protected:
	U8          _exclusionMask;

private:
	
	Material*	_material;				   

	bool		_renderState;
	bool        _noDefaultMaterial;
	bool        _selected;
	I32         _sortKey;
};

#endif