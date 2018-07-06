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

#ifndef _SKINNED_SUBMESH_H_
#define _SKINNED_SUBMESH_H_

#include "SubMesh.h"
class SceneAnimator;
class SkinnedSubMesh : public SubMesh {
public:
	SkinnedSubMesh(const std::string& name) : SubMesh(name,Object3D::OBJECT_FLAG_SKINNED),
									          _skeletonAvailable(false),
									          _softwareSkinning(false),
											  _deltaTime(0),
											  _currentAnimationID(0),
											  _currentFrameIndex(0),
											  _animator(NULL)
	{
	}

	~SkinnedSubMesh();

public:
	inline vectorImpl<mat4<F32> >& GetTransforms(){ return _transforms; }
	bool createAnimatorFromScene(const aiScene* scene,U8 subMeshPointer);
	void renderSkeleton(SceneGraphNode* const sgn);
	void updateBBatCurrentFrame(SceneGraphNode* const sgn);
	void updateAnimations(D32 timeIndex);
	void postLoad(SceneGraphNode* const sgn);
	void onDraw();
	void preFrameDrawEnd(SceneGraphNode* const sgn);
	void updateTransform(SceneGraphNode* const sgn);
	void setSpecialShaderConstants(ShaderProgram* const shader);
private:
	/// Animation player to animate the mesh if necessary
	SceneAnimator* _animator;
	vectorImpl<vec3<F32> > _origVerts;
	/// bone transforms for the entire submesh
	vectorImpl<mat4<F32> > _transforms;
	D32 _deltaTime;
	bool _skeletonAvailable; ///<Does the mesh have a valid skeleton?
	bool _softwareSkinning;
	/// Current animation ID
	U32 _currentAnimationID;
	/// Current animation frame wrapped in animation time [0 ... 1]
	U32 _currentFrameIndex;
	///BoundingBoxes for every frame
	typedef Unordered_map<U32 /*frame index*/, BoundingBox>  boundingBoxPerFrame;
	boundingBoxPerFrame tempHolder;
	///store a map of bounding boxes for every animation at every frame
	Unordered_map<U32 /*animation ID*/, boundingBoxPerFrame> _boundingBoxes;
};
#endif