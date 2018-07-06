#ifndef _BONES_H_
#define _BONES_H_

#include "resource.h"
#include "Utility/Headers/Quaternion.h" 

#define MAX_BONE_CHILD_COUNT 20

typedef struct
 {
 	U16  time;				//At what time index does this Keyframe apply
 	F32  length;			//What length should the bone have at this frame
	Quaternion orientation; //Bone orientation at current frame expressed as a quaternion
	vec3 position;          //The bone's final position at this frame
 } Keyframe;

struct Bone
{
	I16 _id, _childCount;
	mat4 _absMatrix, _relMatrix; //Transformation matrices for the current bone
	F32 _length;
	Bone *_parent, *_child[MAX_BONE_CHILD_COUNT]; 
	Keyframe *_animation; //Animation for the current bone;
};

Bone *boneAddChild(Bone *root, I16 id, const mat4& _absMatrix);
Bone *boneFreeTree(Bone *root);
Bone *boneFindById(Bone *root, I16 id);

#endif