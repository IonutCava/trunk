/*
   Copyright (c) 2014 DIVIDE-Studio
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

/*Code references:
    http://nolimitsdesigns.com/game-design/open-asset-import-library-animation-loader/
*/

#ifndef ANIMATION_CONTROLLER_H_
#define ANIMATION_CONTROLLER_H_

#include <fstream>
#include <boost/cstdint.hpp>
#include "AnimationEvaluator.h"

struct aiNode;
struct aiScene;
class ShaderBuffer;
class SceneAnimator{
public:

    SceneAnimator(): _skeleton(0)
    {
        _boneTransformBuffer[0] = _boneTransformBuffer[1] = nullptr;
    }

    ~SceneAnimator()
    {
        Release(); 
    }

    /// this must be called to fill the SceneAnimator with valid data
    bool Init(const aiScene* pScene, U8 meshPointer);
    ///Only called if the SceneGraph detected a transformation change
    inline void setGlobalMatrix(const mat4<F32>& matrix) {	_rootTransformRender = matrix; } // update root matrix for mesh;
    /// frees all memory and initializes everything to a default state
    void Release();
    void Save(std::ofstream& file);
    void Load(std::ifstream& file);
    /// lets the caller know if there is a skeleton present
    inline bool HasSkeleton() const { return !_bones.empty(); }
    /// the next two functions are good if you want to change the direction of the current animation. You could use a forward walking animation and reverse it to get a walking backwards
    inline void PlayAnimationForward(I32 animationIndex) { _animations[animationIndex]._playAnimationForward = true; }
    inline void PlayAnimationBackward(I32 animationIndex) { _animations[animationIndex]._playAnimationForward = false; }
    ///this function will adjust the current animations speed by a percentage. So, passing 100, would do nothing, passing 50, would decrease the speed by half, and 150 increase it by 50%
    inline void AdjustAnimationSpeedBy(I32 animationIndex, const D32 percent) { _animations[animationIndex]._ticksPerSecond *= percent / 100.0f; }
    ///This will set the animation speed
    inline void AdjustAnimationSpeedTo(I32 animationIndex, const D32 tickspersecond) { _animations[animationIndex]._ticksPerSecond = tickspersecond; }
    /// get the animationspeed... in ticks per second
    inline F32 GetAnimationSpeed(I32 animationIndex) const { return _animations[animationIndex]._ticksPerSecond; }
    /// get the transforms needed to pass to the vertex shader. This will wrap the dt value passed, so it is safe to pass 50000000 as a valid number
    inline vectorImpl<mat4<F32> >& GetTransforms(I32 animationIndex, const D32 dt){ return _animations[animationIndex].GetTransforms(dt); }
    inline vectorImpl<mat4<F32> >& GetTransformsByIndex(I32 animationIndex, U32 index){ return _animations[animationIndex]._transforms[index]; }
    inline I32 GetFrameIndex(I32 animationIndex) const { return _animations[animationIndex].GetFrameIndex(); }
    inline U32 GetFrameCount(I32 animationIndex) const { return _animations[animationIndex].GetFrameCount(); }
    inline const vectorImpl<AnimEvaluator>& GetAnimations() const { return _animations; }
    inline const std::string& GetAnimationName(I32 animationIndex) const { return _animations[animationIndex]._name; }
    inline bool GetAnimationID(const std::string& animationName, U32& id){
        Unordered_map<std::string, U32>::iterator itr = _animationNameToId.find(animationName);
        if (itr != _animationNameToId.end()) {
            id = itr->second;
            return true;
        }
        return false;
    }
    ///GetBoneTransform will return the matrix of the bone given its name and the time. be careful with this to make sure and send the correct dt. If the dt is different from what the model is currently at, the transform will be off
    inline const mat4<F32>& GetBoneTransform(I32 animationIndex, const D32 dt, const std::string& bname) { 
        I32 bindex = GetBoneIndex(bname);
        if (bindex == -1) 
            return _cacheIdentity;
        return _animations[animationIndex].GetTransforms(dt)[bindex]; 
    }

    /// same as above, except takes the index
    inline const mat4<F32>& GetBoneTransform(I32 animationIndex, const D32 dt, U32 bindex) { return _animations[animationIndex].GetTransforms(dt)[bindex]; }
	/// get the bone's global transform
	inline const mat4<F32>& GetBoneOffsetTransform(const std::string& bname) {
		I32 bindex=GetBoneIndex(bname);
		if(bindex != -1) {
			AnimUtils::TransformMatrix(_cacheIdentity, _bonesByName[bname]->_offsetMatrix);
		}
		return _cacheIdentity;
	}

    vectorImpl<AnimEvaluator> _animations;// a vector that holds each animation
    Bone* GetBoneByName(const std::string& name) const;
	///GetBoneIndex will return the index of the bone given its name. The index can be used to index directly into the vector returned from GetTransform
    I32 GetBoneIndex(const std::string& bname) const;
    I32 RenderSkeleton(I32 animationIndex, const D32 dt);

    size_t GetBoneCount() const { return _bones.size(); }

    ShaderBuffer* getBoneDataBuffer(U8 index) const { return _boneTransformBuffer[index]; }

private:

    ///I/O operations
    void SaveSkeleton(std::ofstream& file, Bone* pNode);
    Bone* LoadSkeleton(std::ifstream& file, Bone* pNode);

    void UpdateTransforms(Bone* pNode);
    void Calculate(I32 animationIndex, const D32 pTime);
    void CalcBoneMatrices();
    /// Calculates the global transformation matrix for the given internal node
    void CalculateBoneToWorldTransform(Bone* pInternalNode);
    void ExtractAnimations(const aiScene* pScene);
    /// Recursively creates an internal node structure matching the current scene and animation.
    Bone* CreateBoneTree( aiNode* pNode, Bone* parent);

    I32 SubmitSkeletonToGPU(I32 animationIndex, U32 frameIndex);
    I32 CreateSkeleton(Bone* piNode, const aiMatrix4x4& parent, vectorImpl<vec3<F32> >& pointsA, vectorImpl<vec3<F32> >& pointsB, vectorImpl<vec4<U8> >& colors);

private:
    Bone* _skeleton;/** Root node of the internal scene structure */

    Unordered_map<std::string, Bone*> _bonesByName;/** Name to node map to quickly find nodes by their name */
    Unordered_map<std::string, U32>   _bonesToIndex;/** Name to node map to quickly find nodes by their name */
    Unordered_map<std::string, U32>   _animationNameToId;// find animations quickly

    vectorImpl<Bone*> _bones;// DO NOT DELETE THESE when the destructor runs... THEY ARE JUST REFERENCES!!
    vectorImpl<aiMatrix4x4 > _transforms;// temp array of transforms

    typedef Unordered_map<U32/*frameIndex*/, vectorImpl<vec3<F32> >> pointMap;
    typedef Unordered_map<U32/*animationId*/, pointMap > pointCollection;
    typedef Unordered_map<U32/*frameIndex*/, vectorImpl<vec4<U8> >> colorMap;
    typedef Unordered_map<U32/*animationId*/, colorMap > colorCollection;
    pointCollection _pointsA;
    pointCollection _pointsB;
    colorCollection _colors;
    mat4<F32>  _cacheIdentity;
    mat4<F32>  _rootTransformRender;

    ShaderBuffer* _boneTransformBuffer[2];
};

#endif
