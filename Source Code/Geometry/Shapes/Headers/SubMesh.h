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
struct aiScene;
class SubMesh : public Object3D {
public:
	SubMesh(const std::string& name, ObjectFlag flag = OBJECT_FLAG_NONE) :
									   Object3D(name,SUBMESH,TRIANGLES,flag),
									   _visibleToNetwork(true),
									   _render(true),
									   _id(0),
									   _parentMesh(NULL)
	{
	}

	virtual ~SubMesh();

	bool unload() { return SceneNode::unload(); }

	bool computeBoundingBox(SceneGraphNode* const sgn);
	virtual void updateTransform(SceneGraphNode* const sgn);

	inline U32  getId() {return _id;}
	/// When loading a submesh, the ID is the node index from the imported scene
	/// scene->mMeshes[n] == (SubMesh with _id == n)
	inline void setId(U32 id) {_id = id;}
	virtual void postLoad(SceneGraphNode* const sgn);
	inline Mesh* getParentMesh() {return _parentMesh;}

	virtual void onDraw(const RenderStage& currentStage);
	virtual void preFrameDrawEnd(SceneGraphNode* const sgn) {/*nothing yet*/}
	virtual void updateBBatCurrentFrame(SceneGraphNode* const sgn);
	/// Called from SceneGraph "sceneUpdate"
	virtual void sceneUpdate(const D32 deltaTime,SceneGraphNode* const sgn, SceneState& sceneState);

	inline void setSceneMatrix(const mat4<F32>& sceneMatrix){ _sceneRootMatrix = sceneMatrix; }

protected:
	friend class Mesh;
	inline void setParentMesh(Mesh* const parentMesh) {_parentMesh = parentMesh;}
	friend class DVDConverter;
	mat4<F32> _sceneRootMatrix;

private:
	bool _visibleToNetwork;
	bool _render;
	U32 _id;
	Mesh* _parentMesh;
};

#endif