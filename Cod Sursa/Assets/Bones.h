#include "resource.h"
#include "Utility/Headers/Quaternion.h" 

typedef struct
 {
 	U32  time;				//At what time index does this Keyframe apply
 	F32  length;			//What length should the bone have at this frame
	Quaternion orientation; //Bone orientation at current frame expressed as a quaternion
	vec3 position;          //The bone's final position at this frame
 } Keyframe;

class Bone
{
public:
	Bone(int id, const mat4& absMatrix, const mat4& relMatrix, F32 length) :
	  _id(id),  
	  _absMatrix(absMatrix),
	  _relMatrix(relMatrix),
	  _length(length)
	  {}
private:
 int _id;
 mat4 _absMatrix, _relMatrix; //Transformation matrices for the current bone
 F32 _length;
 Bone *_parent, *_child, *_brother; 
 Keyframe *_animation; //Animation for the current bone;
};