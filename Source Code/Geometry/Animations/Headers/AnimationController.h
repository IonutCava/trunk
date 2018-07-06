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
/*Code references:
	http://nolimitsdesigns.com/game-design/open-asset-import-library-animation-loader/
*/


#ifndef ANIMATION_CONTROLLER_H_
#define ANIMATION_CONTROLLER_H_

#include <tuple>
#include "Bone.h"
#include <assimp/anim.h>

class AnimationChannel{
public:
	std::string Name;
	std::vector<aiVectorKey > mPositionKeys;
	std::vector<aiQuatKey   > mRotationKeys;
	std::vector<aiVectorKey > mScalingKeys;
};

struct aiAnimation;
class AnimationEvaluator{
public:

	AnimationEvaluator(): mLastTime(0.0f), TicksPerSecond(0.0f), Duration(0.0f), PlayAnimationForward(true) {}
	AnimationEvaluator( const aiAnimation* pAnim);
	void Evaluate( F32 pTime, std::map<std::string, Bone*>& bones);
	void Save(std::ofstream& file);
	void Load(std::ifstream& file);
	std::vector<mat4<F32> >& GetTransforms(F32 dt){ return Transforms[GetFrameIndexAt(dt)]; }
	U32 GetFrameIndexAt(F32 time);

	std::string Name;
	std::vector<AnimationChannel> Channels;
	bool PlayAnimationForward;// play forward == true, play backward == false
	F32 mLastTime, TicksPerSecond, Duration;	
	std::vector<tuple_impl<U32, U32, U32> > mLastPositions;
	std::vector<std::vector<mat4<F32> >> Transforms;//, QuatTransforms;/** Array to return transformations results inside. */
};

struct aiNode; 
struct aiScene;
class AnimationController{
public:

	AnimationController(): Skeleton(0), CurrentAnimIndex(-1) {}
	~AnimationController(){ Release(); }

	void Init(const aiScene* pScene);// this must be called to fill the AnimationController with valid data
	void Release();// frees all memory and initializes everything to a default state
	void Save(std::ofstream& file);
	void Load(std::ifstream& file);
	inline bool HasSkeleton() const   { return !Bones.empty(); }// lets the caller know if there is a skeleton present
	inline bool HasAnimations() const { return !Animations.empty(); }
	// the set animation returns whether the animation changed or is still the same. 
	bool SetAnimIndex(I32 pAnimIndex);// this takes an index to set the current animation to
	bool SetAnimation(const std::string& name);// this takes a string to set the animation to, i.e. SetAnimation("Idle");
	// the next two functions are good if you want to change the direction of the current animation. You could use a forward walking animation and reverse it to get a walking backwards
	inline void PlayAnimationForward() { Animations[CurrentAnimIndex].PlayAnimationForward = true; }
	inline void PlayAnimationBackward() { Animations[CurrentAnimIndex].PlayAnimationForward = false; }
	//this function will adjust the current animations speed by a percentage. So, passing 100, would do nothing, passing 50, would decrease the speed by half, and 150 increase it by 50%
	inline void AdjustAnimationSpeedBy(F32 percent) { Animations[CurrentAnimIndex].TicksPerSecond*=percent/100.0f; }
	//This will set the animation speed
	inline void AdjustAnimationSpeedTo(F32 tickspersecond) { Animations[CurrentAnimIndex].TicksPerSecond=tickspersecond; }
	// get the animationspeed...
	inline F32 GetAnimationSpeed() const { return Animations[CurrentAnimIndex].TicksPerSecond; }
	// get the transforms needed to pass to the vertex shader. This will wrap the dt value passed, so it is safe to pass 50000000 as a valid number
	inline std::vector<mat4<F32> >& GetTransforms(F32 dt){ return Animations[CurrentAnimIndex].GetTransforms(dt); }

	inline I32 GetAnimationIndex() const { return CurrentAnimIndex; }
	inline std::string GetAnimationName() const { return Animations[CurrentAnimIndex].Name;  }

	std::vector<AnimationEvaluator> Animations;// a std::vector that holds each animation 
	I32 CurrentAnimIndex;/** Current animation index */

protected:		

	
	Bone* Skeleton;/** Root node of the internal scene structure */
	std::map<std::string, Bone*> BonesByName;/** Name to node map to quickly find nodes by their name */
	std::map<std::string, U32> AnimationNameToId;// find animations quickly
	std::vector<Bone*> Bones;// DO NOT DELETE THESE when the destructor runs... THEY ARE JUST COPIES!!
	std::vector<mat4<F32> > Transforms;// temp array of transfrorms

	void SaveSkeleton(std::ofstream& file, Bone* pNode);
	Bone* LoadSkeleton(std::ifstream& file, Bone* pNode);

	void UpdateTransforms(Bone* pNode);
	void CalculateBoneToWorldTransform(Bone* pInternalNode);/** Calculates the global transformation matrix for the given internal node */

	void Calculate( F32 pTime);
	void CalcBoneMatrices();

	void ExtractAnimations(const aiScene* pScene);
	Bone* CreateBoneTree( aiNode* pNode, Bone* pParent);// Recursively creates an internal node structure matching the current scene and animation. 
	
};


#endif