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
#include "AnimationEvaluator.h"

struct aiNode;
struct aiScene;

namespace Divide {

class SceneAnimator{
public:
    typedef hashMapImpl<I32/*frameIndex*/, vectorAlg::vecSize/*vectorIntex*/> LineMap;
    typedef hashMapImpl<I32/*animationId*/, LineMap> LineCollection;

    SceneAnimator(): _skeleton(0)
    {
    }

    ~SceneAnimator()
    {
        Release(); 
    }

    /// This must be called to fill the SceneAnimator with valid data
    bool Init(const aiScene* pScene, U8 meshPointer);
    /// Frees all memory and initializes everything to a default state
    void Release();
    void Save(std::ofstream& file);
    void Load(std::ifstream& file);
    /// Lets the caller know if there is a skeleton present
    inline bool HasSkeleton() const { return !_bones.empty(); }
    /// The next two functions are good if you want to change the direction of the current animation. 
    /// You could use a forward walking animation and reverse it to get a walking backwards
    inline void PlayAnimationForward(I32 animationIndex) { _animations[animationIndex]._playAnimationForward = true; }
    inline void PlayAnimationBackward(I32 animationIndex) { _animations[animationIndex]._playAnimationForward = false; }
    /// This function will adjust the current animations speed by a percentage.
    /// So, passing 100, would do nothing, passing 50, would decrease the speed by half, and 150 increase it by 50%
    inline void AdjustAnimationSpeedBy(I32 animationIndex, const D32 percent) {
        _animations[animationIndex]._ticksPerSecond *= percent / 100.0f; 
    }
    /// This will set the animation speed
    inline void AdjustAnimationSpeedTo(I32 animationIndex, const D32 tickspersecond) { 
        _animations[animationIndex]._ticksPerSecond = tickspersecond; 
    }
    /// Get the animationspeed... in ticks per second
    inline F32 GetAnimationSpeed(I32 animationIndex) const { 
        return _animations[animationIndex]._ticksPerSecond; 
    }
    /// Get the transforms needed to pass to the vertex shader. 
    /// This will wrap the dt value passed, so it is safe to pass 50000000 as a valid number
    inline vectorImpl<mat4<F32> >& GetTransforms(I32 animationIndex, const D32 dt) { 
        return _animations[animationIndex].GetTransforms(dt); 
    }
    inline vectorImpl<mat4<F32> >& GetTransformsByIndex(I32 animationIndex, U32 index) { 
        return _animations[animationIndex]._transforms[index]; 
    }
    inline I32 GetFrameIndex(I32 animationIndex) const { 
        return _animations[animationIndex].GetFrameIndex(); 
    }
    inline U32 GetFrameCount(I32 animationIndex) const { 
        return _animations[animationIndex].GetFrameCount(); 
    }
    inline const vectorImpl<AnimEvaluator>& GetAnimations() const { 
        return _animations; 
    }
    inline const stringImpl& GetAnimationName(I32 animationIndex) const { 
        return _animations[animationIndex]._name; 
    }
    inline bool GetAnimationID(const stringImpl& animationName, U32& id){
        hashMapImpl<stringImpl, U32>::iterator itr = _animationNameToId.find(animationName);
        if (itr != _animationNameToId.end()) {
            id = itr->second;
            return true;
        }
        return false;
    }
    /// GetBoneTransform will return the matrix of the bone given its name and the time. 
    /// Be careful with this to make sure and send the correct dt. If the dt is different from what the model is currently at,
    /// the transform will be off
    inline const mat4<F32>& GetBoneTransform(I32 animationIndex, const D32 dt, const stringImpl& bname) { 
        I32 bindex = GetBoneIndex(bname);
        if (bindex == -1) 
            return _cacheIdentity;
        return _animations[animationIndex].GetTransforms(dt)[bindex]; 
    }

    /// Same as above, except takes the index
    inline const mat4<F32>& GetBoneTransform(I32 animationIndex, const D32 dt, U32 bindex) { 
        return _animations[animationIndex].GetTransforms(dt)[bindex]; 
    }
    /// Get the bone's global transform
    inline const mat4<F32>& GetBoneOffsetTransform(const stringImpl& bname) {
        I32 bindex=GetBoneIndex(bname);
        if(bindex != -1) {
            AnimUtils::TransformMatrix(_bonesByName[bname]->_offsetMatrix, _cacheIdentity);
        }
        return _cacheIdentity;
    }

    /// A vector that holds each animation
    vectorImpl<AnimEvaluator> _animations;
    Bone* GetBoneByName(const stringImpl& name) const;
    /// GetBoneIndex will return the index of the bone given its name. 
    /// The index can be used to index directly into the vector returned from GetTransform
    I32 GetBoneIndex(const stringImpl& bname) const;
    const vectorImpl<Line >& getSkeletonLines(I32 animationIndex, const D32 dt);

    size_t GetBoneCount() const { return _bones.size(); }

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

    I32 CreateSkeleton(Bone* piNode, const aiMatrix4x4& parent, vectorImpl<Line >& lines);

private:
    Bone* _skeleton;/** Root node of the internal scene structure */

    hashMapImpl<stringImpl, Bone*> _bonesByName;/** Name to node map to quickly find nodes by their name */
    hashMapImpl<stringImpl, U32>   _bonesToIndex;/** Name to node map to quickly find nodes by their name */
    hashMapImpl<stringImpl, U32>   _animationNameToId;// find animations quickly

    vectorImpl<Bone*> _bones;// DO NOT DELETE THESE when the destructor runs... THEY ARE JUST REFERENCES!!
    vectorImpl<aiMatrix4x4 > _transforms;// temp array of transforms

    mat4<F32>      _cacheIdentity;
    LineCollection _skeletonLines;
    vectorImpl<vectorImpl<Line >> _skeletonLinesContainer;

};

}; //namespace Divide

#endif
