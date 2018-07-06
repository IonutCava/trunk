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

#ifndef _SKINNED_SUBMESH_H_
#define _SKINNED_SUBMESH_H_

#include "SubMesh.h"
class SceneAnimator;
class SkinnedSubMesh : public SubMesh {
public:
	SkinnedSubMesh(const std::string& name) : SubMesh(name,Object3D::OBJECT_FLAG_SKINNED),
									          _skeletonAvailable(false),
									          _softwareSkinning(false),
											  _playAnimations(true),
											  _elapsedTime(0.0),
											  _currentAnimationID(0),
											  _currentFrameIndex(0),
											  _animator(NULL)
	{
	}

	~SkinnedSubMesh();

public:
	bool createAnimatorFromScene(const aiScene* scene,U8 subMeshPointer);
	void renderSkeleton(SceneGraphNode* const sgn);
	void updateBBatCurrentFrame(SceneGraphNode* const sgn);
	/// Called from SceneGraph "sceneUpdate"
	void sceneUpdate(const D32 deltaTime,SceneGraphNode* const sgn, SceneState& sceneState);
	void updateAnimations(D32 timeIndex,SceneGraphNode* const sgn);
	void postLoad(SceneGraphNode* const sgn);
	void preFrameDrawEnd(SceneGraphNode* const sgn);
	void updateTransform(SceneGraphNode* const sgn);
	SceneAnimator* const getAnimator() {return _animator;}
	mat4<F32> getCurrentBoneTransform(const std::string& name);

private:
	/// Animation player to animate the mesh if necessary
	SceneAnimator* _animator;
	vectorImpl<vec3<F32> > _origVerts;
	vectorImpl<vec3<F32> > _origNorms;
	D32 _elapsedTime;
	bool _skeletonAvailable; ///<Does the mesh have a valid skeleton?
	bool _softwareSkinning;
	bool _playAnimations;
	/// Current animation ID
	U32 _currentAnimationID;
	/// Current animation frame wrapped in animation time [0 ... 1]
	U32 _currentFrameIndex;
	///BoundingBoxes for every frame
	typedef Unordered_map<U32 /*frame index*/, BoundingBox>  boundingBoxPerFrame;
	boundingBoxPerFrame _bbsPerFrame;
	///store a map of bounding boxes for every animation at every frame
	Unordered_map<U32 /*animation ID*/, boundingBoxPerFrame> _boundingBoxes;
};
#endif