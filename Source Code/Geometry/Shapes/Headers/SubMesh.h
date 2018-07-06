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

#ifndef _SUB_MESH_H_
#define _SUB_MESH_H_

#include "Object3D.h"

/*
DIVIDE-Engine: 21.10.2010 (Ionut Cava)

A SubMesh is a geometry wrapper used to build a mesh. Just like a mesh, it can be rendered locally or across
the server or disabled from rendering alltogheter. 

Objects created from this class have theyr position in relative space based on the parent mesh position.
(Same for scale,rotation and so on).

The SubMesh is composed of a VBO object that contains vertx,normal and textcoord data, a vector of materials,
and a name.
*/

#include "core.h"
#include "Hardware/Video/Buffers/VertexBufferObject/Headers/VertexBufferObject.h"

class Mesh;
class SceneAnimator;
struct aiScene;
class SubMesh : public Object3D {

public:
	SubMesh(const std::string& name) : Object3D(name,SUBMESH),
									   _visibleToNetwork(true),
									   _render(true),
									   _id(0),
									   _parentMesh(NULL),
									   _renderSkeleton(false),
									   _softwareSkinning(false),
							           _deltaTime(0),
									   _hasAnimations(false),
									   _currentAnimationID(-1),
									   _currentFrameIndex(-1),
									   _animator(NULL)
	{
		/// 3D objects usually requires the VBO to be recomputed on creation. mesh objects do not.
		_refreshVBO = false;
	}

	~SubMesh();

	bool unload() { return SceneNode::unload(); }

	bool computeBoundingBox(SceneGraphNode* const sgn);
	void updateTransform(SceneGraphNode* const sgn);

	inline U32  getId() {return _id;}
	/// When loading a submesh, the ID is the node index from the imported scene
	/// scene->mMeshes[n] == (SubMesh with _id == n)
	inline void setId(U32 id) {_id = id;}
	void postLoad(SceneGraphNode* const sgn);
	inline Mesh* getParentMesh() {return _parentMesh;}

	void onDraw();

	/// Called from SceneGraph "sceneUpdate"
	inline std::vector<mat4<F32> >& GetTransforms(){ return _transforms; }

	void sceneUpdate(D32 sceneTime);
	bool createAnimatorFromScene(const aiScene* scene,U8 subMeshPointer);
	void renderSkeleton(SceneGraphNode* const sgn);
	void updateBBatCurrentFrame(SceneGraphNode* const sgn);

	inline void setSceneMatrix(const mat4<F32>& sceneMatrix){ _sceneRootMatrix = sceneMatrix; }

protected:
	friend class Mesh;
	inline void setParentMesh(Mesh* const parentMesh) {_parentMesh = parentMesh;}
		   void updateAnimations(D32 timeIndex);
	bool _hasAnimations;
	friend class DVDConverter;
	mat4<F32> _sceneRootMatrix;

private:
	bool _visibleToNetwork, _render;
	U32 _id;
	Mesh* _parentMesh;
	std::vector<vec3<F32> > _origVerts;
	/// Animation player to animate the mesh if necessary
	SceneAnimator* _animator;
	/// bone transforms for the entire submesh
	std::vector<mat4<F32> > _transforms;
	D32 _deltaTime;
	bool _renderSkeleton;
	bool _softwareSkinning;
	/// Current animation ID
	U32 _currentAnimationID;
	/// Current animation frame wrapped in animation time [0 ... 1]
	U32 _currentFrameIndex;
	///BoundingBoxes for every frame
	typedef unordered_map<U32 /*frame index*/, BoundingBox>  boundingBoxPerFrame;
	boundingBoxPerFrame tempHolder;
	///store a map of bounding boxes for every animation at every frame
	unordered_map<U32 /*animation ID*/, boundingBoxPerFrame> _boundingBoxes;
};

#endif