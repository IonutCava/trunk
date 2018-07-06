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
	friend class SceneGraphNode;
	friend class RenderQueue;
public:
	SceneNode() : Resource(),
				 _material(NULL),
				 _renderState(true),
				 _noDefaultMaterial(false),
	             _sortKey(0){}
	SceneNode(std::string name) : Resource(name),
								  _material(NULL),
								  _renderState(true),
								  _noDefaultMaterial(false),
								  _sortKey(0){}

	virtual ~SceneNode() {}
	/*Rendering/Processing*/
	virtual void render(SceneGraphNode* node) = 0; //Sounds are played, geometry is displayed etc.
			void setRenderState(bool state) {_renderState = state;}
			bool getRenderState() {return _renderState;}
	/*//Rendering/Processing*/	

	virtual	bool			unload();

	virtual bool	        isInView(bool distanceCheck,BoundingBox& boundingBox);
    virtual	void			setMaterial(Material* m);
			void			clearMaterials();
		    Material*		getMaterial();
	virtual	void            prepareMaterial();
	virtual	void            releaseMaterial();
			SceneGraphNode* getSceneGraphNode();
			void            setSceneGraphNode(const std::string& name) {_sceneGraphNodeName = name;}
	virtual	bool    computeBoundingBox(SceneGraphNode* node);
	virtual void    onDraw();
	virtual void    postLoad(SceneGraphNode* node) = 0; //Post insertion calls (Use this to setup child objects during creation)
	void    useDefaultMaterial(bool state) {_noDefaultMaterial = !state;}
	

	inline	void	setSelected(bool state)  {_selected = state;}
	inline	bool    isSelected()			 {return _selected;}
	virtual void    createCopy();
	virtual void    removeCopy();
private:
	
	Material*	_material;				   

	bool		_renderState,_noDefaultMaterial;
	bool        _selected;
	F32         _sortKey;
	std::string _sceneGraphNodeName;

};

#endif