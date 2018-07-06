#include "Headers/AnimationController.h"
#include "Core/Math/Headers/Quaternion.h"
#include "Hardware/Video/GFXDevice.h"
#include <assimp/scene.h>

void TransformMatrix(mat4<F32>& out,const aiMatrix4x4& in){// there is some type of alignment issue with my mat4 and the aimatrix4x4 class, so the copy must be manually
	//out._11=in.a1;
	//out._12=in.a2;
	//out._13=in.a3;
	//out._14=in.a4;

	//out._21=in.b1;
	//out._22=in.b2;
	//out._23=in.b3;
	//out._24=in.b4;

	//out._31=in.c1;
	//out._32=in.c2;
	//out._33=in.c3;
	//out._34=in.c4;

	//out._41=in.d1;
	//out._42=in.d2;
	//out._43=in.d3;
	//out._44=in.d4;

	out.setCol(0,vec4<F32>(in.a1,in.b1,in.c1,in.d1));
	out.setCol(2,vec4<F32>(in.a2,in.b2,in.c2,in.d2));
	out.setCol(3,vec4<F32>(in.a3,in.b3,in.c3,in.d3));
	out.setCol(4,vec4<F32>(in.a4,in.b4,in.c4,in.d4));
}

// ------------------------------------------------------------------------------------------------
// Constructor on a given animation. 
AnimationEvaluator::AnimationEvaluator( const aiAnimation* pAnim) {
	mLastTime = 0.0;
	TicksPerSecond = static_cast<F32>(pAnim->mTicksPerSecond != 0.0f ? pAnim->mTicksPerSecond : 100.0f);
	Duration = static_cast<F32>(pAnim->mDuration);
	Name = pAnim->mName.data;
	D_PRINT_FN("Creating Animation named: [ %s ]", Name.c_str());
	Channels.resize(pAnim->mNumChannels);
	for( U32 a = 0; a < pAnim->mNumChannels; a++){		
		Channels[a].Name = pAnim->mChannels[a]->mNodeName.data;
		for(U32 i(0); i< pAnim->mChannels[a]->mNumPositionKeys; i++) Channels[a].mPositionKeys.push_back(pAnim->mChannels[a]->mPositionKeys[i]);
		for(U32 i(0); i< pAnim->mChannels[a]->mNumRotationKeys; i++) Channels[a].mRotationKeys.push_back(pAnim->mChannels[a]->mRotationKeys[i]);
		for(U32 i(0); i< pAnim->mChannels[a]->mNumScalingKeys; i++) Channels[a].mScalingKeys.push_back(pAnim->mChannels[a]->mScalingKeys[i]);
	}
	mLastPositions.resize( pAnim->mNumChannels, make_tuple_impl( 0, 0, 0));
	D_PRINT_FN("Finished Creating Animation named: [ %s ] ", Name.c_str());
}

U32 AnimationEvaluator::GetFrameIndexAt(F32 ptime){
	// get a [0.f ... 1.f) value by allowing the percent to wrap around 1
	ptime *= TicksPerSecond;
	
	F32 time = 0.0f;
	if( Duration > 0.0)
		time = fmod( ptime, Duration);

	F32 percent = time / Duration;
	if(!PlayAnimationForward) percent= (percent-1.0f)*-1.0f;// this will invert the percent so the animation plays backwards
	return static_cast<U32>(( static_cast<F32>(Transforms.size()) * percent));
}

void AnimationEvaluator::Save(std::ofstream& file){
	U32 nsize = static_cast<U32>(Name.size());
	file.write(reinterpret_cast<char*>(&nsize), sizeof(U32));// the size of the animation name
	file.write(Name.c_str(), nsize);// the size of the animation name
	file.write(reinterpret_cast<char*>(&Duration), sizeof(Duration));// the duration
	file.write(reinterpret_cast<char*>(&TicksPerSecond), sizeof(TicksPerSecond));// the number of ticks per second
	nsize = static_cast<U32>(Channels.size());// number of animation channels,
	file.write(reinterpret_cast<char*>(&nsize), sizeof(U32));// the number animation channels
	for(size_t j(0); j< Channels.size(); j++){// for each channel
		nsize =static_cast<U32>(Channels[j].Name.size());
		file.write(reinterpret_cast<char*>(&nsize), sizeof(U32));// the size of the name
		file.write(Channels[j].Name.c_str(), nsize);// the size of the animation name

		nsize =static_cast<U32>(Channels[j].mPositionKeys.size());
		file.write(reinterpret_cast<char*>(&nsize), sizeof(U32));// the number of position keys
		for(size_t i(0); i< nsize; i++){// for each channel
			file.write(reinterpret_cast<char*>(&Channels[j].mPositionKeys[i].mTime), sizeof(Channels[j].mPositionKeys[i].mTime));// pos key
			file.write(reinterpret_cast<char*>(&Channels[j].mPositionKeys[i].mValue), sizeof(Channels[j].mPositionKeys[i].mValue));// pos key
		}

		nsize =static_cast<U32>(Channels[j].mRotationKeys.size());
		file.write(reinterpret_cast<char*>(&nsize), sizeof(U32));// the number of position keys
		for(size_t i(0); i< nsize; i++){// for each channel
			file.write(reinterpret_cast<char*>(&Channels[j].mRotationKeys[i].mTime), sizeof(Channels[j].mRotationKeys[i].mTime));// rot key
			file.write(reinterpret_cast<char*>(&Channels[j].mRotationKeys[i].mValue), sizeof(Channels[j].mRotationKeys[i].mValue));// rot key
		}

		nsize =static_cast<U32>(Channels[j].mScalingKeys.size());
		file.write(reinterpret_cast<char*>(&nsize), sizeof(U32));// the number of position keys
		for(size_t i(0); i< nsize; i++){// for each channel
			file.write(reinterpret_cast<char*>(&Channels[j].mScalingKeys[i].mTime), sizeof(Channels[j].mScalingKeys[i].mTime));// rot key
			file.write(reinterpret_cast<char*>(&Channels[j].mScalingKeys[i].mValue), sizeof(Channels[j].mScalingKeys[i].mValue));// rot key
		}

	}
}

void AnimationEvaluator::Load(std::ifstream& file){
	U32 nsize = 0;
	file.read(reinterpret_cast<char*>(&nsize), sizeof(U32));// the size of the animation name
	char temp[250];
	file.read(temp, nsize);// the size of the animation name
	temp[nsize]=0;// null char
	Name=temp;
	D_PRINT_FN("Creating Animation named: [ %s ]",Name.c_str());
	file.read(reinterpret_cast<char*>(&Duration), sizeof(Duration));// the duration
	file.read(reinterpret_cast<char*>(&TicksPerSecond), sizeof(TicksPerSecond));// the number of ticks per second
	file.read(reinterpret_cast<char*>(&nsize), sizeof(U32));// the number animation channels
	Channels.resize(nsize);
	for(size_t j(0); j< Channels.size(); j++){// for each channel
		nsize =0;
		file.read(reinterpret_cast<char*>(&nsize), sizeof(U32));// the size of the name
		file.read(temp, nsize);// the size of the animation name
		temp[nsize]=0;// null char
		Channels[j].Name=temp;

		nsize =static_cast<U32>(Channels[j].mPositionKeys.size());
		file.read(reinterpret_cast<char*>(&nsize), sizeof(U32));// the number of position keys
		Channels[j].mPositionKeys.resize(nsize);
		for(size_t i(0); i< nsize; i++){// for each channel
			file.read(reinterpret_cast<char*>(&Channels[j].mPositionKeys[i].mTime), sizeof(Channels[j].mPositionKeys[i].mTime));// pos key
			file.read(reinterpret_cast<char*>(&Channels[j].mPositionKeys[i].mValue), sizeof(Channels[j].mPositionKeys[i].mValue));// pos key
		}

		nsize =static_cast<U32>(Channels[j].mRotationKeys.size());
		file.read(reinterpret_cast<char*>(&nsize), sizeof(U32));// the number of position keys
		Channels[j].mRotationKeys.resize(nsize);
		for(size_t i(0); i< nsize; i++){// for each channel
			file.read(reinterpret_cast<char*>(&Channels[j].mRotationKeys[i].mTime), sizeof(Channels[j].mRotationKeys[i].mTime));// pos key
			file.read(reinterpret_cast<char*>(&Channels[j].mRotationKeys[i].mValue), sizeof(Channels[j].mRotationKeys[i].mValue));// pos key
		}

		nsize =static_cast<U32>(Channels[j].mScalingKeys.size());
		file.read(reinterpret_cast<char*>(&nsize), sizeof(U32));// the number of position keys
		Channels[j].mScalingKeys.resize(nsize);
		for(size_t i(0); i< nsize; i++){// for each channel
			file.read(reinterpret_cast<char*>(&Channels[j].mScalingKeys[i].mTime), sizeof(Channels[j].mScalingKeys[i].mTime));// pos key
			file.read(reinterpret_cast<char*>(&Channels[j].mScalingKeys[i].mValue), sizeof(Channels[j].mScalingKeys[i].mValue));// pos key
		}
	}
	mLastPositions.resize( Channels.size(), make_tuple_impl( 0, 0, 0));
}

// ------------------------------------------------------------------------------------------------
// Evaluates the animation tracks for a given time stamp. 
void AnimationEvaluator::Evaluate( F32 pTime, std::map<std::string, Bone*>& bones) {

	pTime *= TicksPerSecond;
	
	F32 time = 0.0f;
	if( Duration > 0.0)
		time = fmod( pTime, Duration);

	// calculate the transformations for each animation channel
	for( U32 a = 0; a < Channels.size(); a++){
		const AnimationChannel* channel = &Channels[a];
		std::map<std::string, Bone*>::iterator bonenode = bones.find(channel->Name);

		if(bonenode == bones.end()) { 
			D_PRINT_FN("did not find the bone node [ %s ]",channel->Name.c_str());
			continue;
		}

		// ******** Position *****
		aiVector3D presentPosition( 0, 0, 0);
		if( channel->mPositionKeys.size() > 0){
			// Look for present frame number. Search from last position if time is after the last time, else from beginning
			// Should be much quicker than always looking from start for the average use case.
			U32 frame = (time >= mLastTime) ? tuple_get_impl<0>(mLastPositions[a]): 0;
			while( frame < channel->mPositionKeys.size() - 1){
				if( time < channel->mPositionKeys[frame+1].mTime){
					break;
				}
				frame++;
			}

			// interpolate between this frame's value and next frame's value
			U32 nextFrame = (frame + 1) % channel->mPositionKeys.size();
	
			const aiVectorKey& key = channel->mPositionKeys[frame];
			const aiVectorKey& nextKey = channel->mPositionKeys[nextFrame];
			D32 diffTime = nextKey.mTime - key.mTime;
			if( diffTime < 0.0)
				diffTime += Duration;
			if( diffTime > 0) {
				F32 factor = F32( (time - key.mTime) / diffTime);
				presentPosition = key.mValue + (nextKey.mValue - key.mValue) * factor;
			} else {
				presentPosition = key.mValue;
			}
			tuple_get_impl<0>(mLastPositions[a]) = frame;
		}
		// ******** Rotation *********
		aiQuaternion presentRotation( 1, 0, 0, 0);
		if( channel->mRotationKeys.size() > 0)
		{
			U32 frame = (time >= mLastTime) ? tuple_get_impl<1>(mLastPositions[a]) : 0;
			while( frame < channel->mRotationKeys.size()  - 1){
				if( time < channel->mRotationKeys[frame+1].mTime)
					break;
				frame++;
			}

			// interpolate between this frame's value and next frame's value
			U32 nextFrame = (frame + 1) % channel->mRotationKeys.size() ;

			const aiQuatKey& key = channel->mRotationKeys[frame];
			const aiQuatKey& nextKey = channel->mRotationKeys[nextFrame];
			D32 diffTime = nextKey.mTime - key.mTime;
			if( diffTime < 0.0) diffTime += Duration;
			if( diffTime > 0) {
				F32 factor = F32( (time - key.mTime) / diffTime);
				aiQuaternion::Interpolate( presentRotation, key.mValue, nextKey.mValue, factor);
			} else presentRotation = key.mValue;
			tuple_get_impl<1>(mLastPositions[a]) = frame;
		}

		// ******** Scaling **********
		aiVector3D presentScaling( 1, 1, 1);
		if( channel->mScalingKeys.size() > 0) {
			U32 frame = (time >= mLastTime) ? tuple_get_impl<2>(mLastPositions[a]) : 0;
			while( frame < channel->mScalingKeys.size() - 1){
				if( time < channel->mScalingKeys[frame+1].mTime)
					break;
				frame++;
			}

			presentScaling = channel->mScalingKeys[frame].mValue;
			tuple_get_impl<2>(mLastPositions[a]) = frame;
		}

		aiMatrix4x4 mat = aiMatrix4x4( presentRotation.GetMatrix());

		mat.a1 *= presentScaling.x; mat.b1 *= presentScaling.x; mat.c1 *= presentScaling.x;
		mat.a2 *= presentScaling.y; mat.b2 *= presentScaling.y; mat.c2 *= presentScaling.y;
		mat.a3 *= presentScaling.z; mat.b3 *= presentScaling.z; mat.c3 *= presentScaling.z;
		mat.a4 = presentPosition.x; mat.b4 = presentPosition.y; mat.c4 = presentPosition.z;
		/// Transpose matrix for D3D
		if(GFX_DEVICE.getApi() != OpenGL){
			mat.Transpose();
		}
		
		TransformMatrix(bonenode->second->_localTransform, mat);
	}
	mLastTime = time;
}

void AnimationController::Release(){// this should clean everything up 
	CurrentAnimIndex = -1;
	Animations.clear();// clear all animations
	delete Skeleton;// This node will delete all children recursivly
	Skeleton = NULL;// make sure to zero it out
}

void AnimationController::Init(const aiScene* pScene){// this will build the skeleton based on the scene passed to it and CLEAR EVERYTHING
	if(!pScene->HasAnimations()) return;
	Release();
	
	Skeleton = CreateBoneTree( pScene->mRootNode, NULL);
	ExtractAnimations(pScene);
	
	for (U32 i = 0; i < pScene->mNumMeshes;++i){
		const aiMesh* mesh = pScene->mMeshes[i];
		
		for (U32 n = 0; n < mesh->mNumBones;++n){
			const aiBone* bone = mesh->mBones[n];
			std::map<std::string, Bone*>::iterator found = BonesByName.find(bone->mName.data);
			if(found != BonesByName.end()){// FOUND IT!!! woohoo, make sure its not already in the bone list
				bool skip = false;
				for(size_t j(0); j< Bones.size(); j++){
					std::string bname = bone->mName.data;
					if(Bones[j]->_name == bname) {
						skip = true;// already inserted, skip this so as not to insert the same bone multiple times
						break;
					}
				}
				if(!skip){// only insert the bone if it has not already been inserted
					std::string tes = found->second->_name;
					TransformMatrix(found->second->_offset, bone->mOffsetMatrix);
					///Transpose for D3D
					if(GFX_DEVICE.getApi() != OpenGL){
						found->second->_offset.transpose(found->second->_offset);// transpose their matrix to get in the correct format
					}
					Bones.push_back(found->second);
				}
			} 
		}
	}
	Transforms.resize( Bones.size());
	F32 timestep = 1.0f/30.0f;// 30 per second
	for(size_t i(0); i< Animations.size(); i++){// pre calculate the animations
		SetAnimIndex(i);
		F32 dt = 0;
		for(F32 ticks = 0; ticks < Animations[i].Duration; ticks += Animations[i].TicksPerSecond/30.0f){
			dt +=timestep;
			Calculate(dt);
			Animations[i].Transforms.push_back(std::vector<mat4<F32> >());
			std::vector<mat4<F32> >& trans = Animations[i].Transforms.back();
			for( size_t a = 0; a < Transforms.size(); ++a){
				mat4<F32> rotationmat =  Bones[a]->_offset * Bones[a]->_globalTransform;
				trans.push_back(rotationmat);
			}
		}
	}
	D_PRINT_FN("Finished loading animations with [ %d ] bones", Bones.size());
}

void AnimationController::Save(std::ofstream& file){
	// first recursivly save the skeleton
	if(Skeleton)
		SaveSkeleton(file, Skeleton);

	U32 nsize = static_cast<U32>(Animations.size());
	file.write(reinterpret_cast<char*>(&nsize), sizeof(U32));// the number of animations	
	for(U32 i(0); i< nsize; i++){
		Animations[i].Save(file);
	}

	nsize = static_cast<U32>(Bones.size());
	file.write(reinterpret_cast<char*>(&nsize), sizeof(U32));// the number of bones

	for(U32 i(0); i< Bones.size(); i++){
		nsize = static_cast<U32>(Bones[i]->_name.size());
		file.write(reinterpret_cast<char*>(&nsize), sizeof(U32));// the size of the bone name
		file.write(Bones[i]->_name.c_str(), nsize);// the name of the bone
	}
}

void AnimationController::Load(std::ifstream& file){
	Release();// make sure to clear this before writing new data
	Skeleton = LoadSkeleton(file, NULL);
	U32 nsize = 0;
	file.read(reinterpret_cast<char*>(&nsize), sizeof(U32));// the number of animations
	Animations.resize(nsize);
	D_PRINT_FN("Extracting Animations ... ");
	for(U32 i(0); i< nsize; i++){
		Animations[i].Load(file);
	}
	for(U32 i(0); i< Animations.size(); i++){// get all the animation names so I can reference them by name and get the correct id
		AnimationNameToId.insert(std::map<std::string, U32>::value_type(Animations[i].Name, i));
	}
	if(Animations.size() >0) CurrentAnimIndex =0;// set it to the first animation if there are any
	char bname[250];
	file.read(reinterpret_cast<char*>(&nsize), sizeof(U32));// the number of bones
	Bones.resize(nsize);

	for(U32 i(0); i< Bones.size(); i++){	
		file.read(reinterpret_cast<char*>(&nsize), sizeof(U32));// the size of the bone name
		file.read(bname, nsize);// the size of the bone name
		bname[nsize]=0;
		std::map<std::string, Bone*>::iterator found = BonesByName.find(bname);
		Bone* tep = found->second;
		Bones[i]=tep;
	}
	
	Transforms.resize( Bones.size());
	F32 timestep = 1.0f/30.0f;// 30 per second
	for(size_t i(0); i< Animations.size(); i++){// pre calculate the animations
		SetAnimIndex(i);
		F32 dt = 0;
		for(F32 ticks = 0; ticks < Animations[i].Duration; ticks += Animations[i].TicksPerSecond/30.0f){
			dt +=timestep;
			Calculate(dt);
			Animations[i].Transforms.push_back(std::vector<mat4<F32> >());
			std::vector<mat4<F32> >& trans = Animations[i].Transforms.back();
			for( size_t a = 0; a < Transforms.size(); ++a){
				mat4<F32> rotationmat =  Bones[a]->_offset * Bones[a]->_globalTransform;
				trans.push_back(rotationmat);
			}
		}
	}
	D_PRINT_FN("Finished loading animations with  [ %d ] bones", Bones.size());
}

void AnimationController::ExtractAnimations(const aiScene* pScene){
	D_PRINT_FN("Extracting Animations ... ");
	for(size_t i(0); i< pScene->mNumAnimations; i++){
		Animations.push_back(AnimationEvaluator(pScene->mAnimations[i]) );// add the animations
	}
	for(U32 i(0); i< Animations.size(); i++){// get all the animation names so I can reference them by name and get the correct id
		AnimationNameToId.insert(std::map<std::string, U32>::value_type(Animations[i].Name, i));
	}
	CurrentAnimIndex=0;
	SetAnimation("Idle");
}

bool AnimationController::SetAnimation(const std::string& name){
	std::map<std::string, U32>::iterator itr = AnimationNameToId.find(name);
	I32 oldindex = CurrentAnimIndex;
	if(itr !=AnimationNameToId.end()) CurrentAnimIndex = itr->second;
	return oldindex != CurrentAnimIndex;
}

bool AnimationController::SetAnimIndex( I32  pAnimIndex){
	if(pAnimIndex >= (I32)Animations.size()) return false;// no change, or the animations data is out of bounds
	I32 oldindex = CurrentAnimIndex;
	CurrentAnimIndex = pAnimIndex;// only set this after the checks for good data and the object was actually inserted
	return oldindex != CurrentAnimIndex;
}

// ------------------------------------------------------------------------------------------------
// Calculates the node transformations for the scene. 
void AnimationController::Calculate(F32 pTime){
	if( (CurrentAnimIndex < 0) | (CurrentAnimIndex >= (I32)Animations.size()) ) return;// invalid animation
	
	Animations[CurrentAnimIndex].Evaluate( pTime, BonesByName);
	UpdateTransforms(Skeleton);
	//CalcBoneMatrices();
}

void UQTtoUDQ(vec4<F32>* dual, const Quaternion& q, vec4<F32>& tran ) {
    dual[0].x = q.getX() ; 
    dual[0].y = q.getY() ; 
    dual[0].z = q.getZ() ; 
    dual[0].w = q.getW() ; 
    dual[1].x = 0.5f *  ( tran[0] * q.getW() + tran[1] * q.getZ() - tran[2] * q.getY() ) ; 
    dual[1].y = 0.5f *  (-tran[0] * q.getZ() + tran[1] * q.getW() + tran[2] * q.getX() ) ; 
    dual[1].z = 0.5f *  ( tran[0] * q.getY() - tran[1] * q.getX() + tran[2] * q.getW() ) ; 
    dual[1].w = -0.5f * ( tran[0] * q.getX() + tran[1] * q.getY() + tran[2] * q.getZ() ) ; 
}

// ------------------------------------------------------------------------------------------------
// Calculates the bone matrices for the given mesh. 
void AnimationController::CalcBoneMatrices(){
	for( size_t a = 0; a < Transforms.size(); ++a){
		Transforms[a] =  Bones[a]->_offset * Bones[a]->_globalTransform;
		/*
		mat4<F32> rotationmat = Transforms[a];
		quat q;
		q.frommatrix(rotationmat);
		
		vec4<F32> dual[2] ;
		UQTtoUDQ( dual, q, Transforms[a].row3_v);
		QuatTransforms[a].row0_v =dual[0];
		QuatTransforms[a].row1_v =dual[1];
		*/
	}

}

// ------------------------------------------------------------------------------------------------
// Recursively creates an internal node structure matching the current scene and animation.
Bone* AnimationController::CreateBoneTree( aiNode* pNode, Bone* pParent){
	Bone* internalNode = New Bone();// create a node
	internalNode->_name = pNode->mName.data;// get the name of the bone
	internalNode->_parent = pParent; //set the parent, in the case this is theroot node, it will be null

	BonesByName[internalNode->_name] = internalNode;// use the name as a key
	TransformMatrix(internalNode->_localTransform, pNode->mTransformation);
	///Transpose for D3D
	if(GFX_DEVICE.getApi() != OpenGL){
		internalNode->_localTransform.transpose(internalNode->_localTransform);
	}
	internalNode->_originalLocalTransform = internalNode->_localTransform;// a copy saved
	CalculateBoneToWorldTransform(internalNode);

	// continue for all child nodes and assign the created internal nodes as our children
	for( U32 a = 0; a < pNode->mNumChildren; a++){// recursivly call this function on all children
		internalNode->_children.push_back(CreateBoneTree( pNode->mChildren[a], internalNode));
	}
	return internalNode;
}

// ------------------------------------------------------------------------------------------------
// Recursively updates the internal node transformations from the given matrix array
void AnimationController::UpdateTransforms(Bone* pNode) {
	CalculateBoneToWorldTransform( pNode);// update global transform as well
	for_each(Bone* b, pNode->_children){
		// continue for all children
		UpdateTransforms(b);
	}
}

// ------------------------------------------------------------------------------------------------
// Calculates the global transformation matrix for the given internal node
void AnimationController::CalculateBoneToWorldTransform(Bone* child){
	child->_globalTransform = child->_localTransform;
	Bone* parent = child->_parent;
	while( parent ){// this will climb the nodes up along through the parents concentating all the matrices to get the Object to World transform, or in this case, the Bone To World transform
		child->_globalTransform *= parent->_localTransform;
		parent  = parent->_parent;// get the parent of the bone we are working on 
	}
}

void AnimationController::SaveSkeleton(std::ofstream& file, Bone* parent){
	U32 nsize = static_cast<U32>(parent->_name.size());
	file.write(reinterpret_cast<char*>(&nsize), sizeof(U32));// the number of chars
	file.write(parent->_name.c_str(), nsize);// the name of the bone
	file.write(reinterpret_cast<char*>(&parent->_offset), sizeof(parent->_offset));// the bone offsets
	file.write(reinterpret_cast<char*>(&parent->_originalLocalTransform), sizeof(parent->_originalLocalTransform));// original bind pose
	nsize = static_cast<U32>(parent->_children.size());// number of children
	file.write(reinterpret_cast<char*>(&nsize), sizeof(U32));// the number of children
	for_each(Bone* b, parent->_children){
		// continue for all children
		SaveSkeleton(file, b); 
	}
}

Bone* AnimationController::LoadSkeleton(std::ifstream& file, Bone* parent){
	Bone* internalNode = New Bone();// create a node
	internalNode->_parent = parent; //set the parent, in the case this is theroot node, it will be null
	U32 nsize=0;
	file.read(reinterpret_cast<char*>(&nsize), sizeof(U32));// the number of chars
	char temp[250];
	file.read(temp, nsize);// the name of the bone
	temp[nsize]=0;
	internalNode->_name = temp;
	BonesByName[internalNode->_name] = internalNode;// use the name as a key
	file.read(reinterpret_cast<char*>(&internalNode->_offset), sizeof(internalNode->_offset));// the bone offsets
	file.read(reinterpret_cast<char*>(&internalNode->_originalLocalTransform), sizeof(internalNode->_originalLocalTransform));// original bind pose
	
	internalNode->_localTransform = internalNode->_originalLocalTransform;// a copy saved
	CalculateBoneToWorldTransform(internalNode);

	file.read(reinterpret_cast<char*>(&nsize), sizeof(U32));// the number of children

	// continue for all child nodes and assign the created internal nodes as our children
	for( U32 a = 0; a < nsize && file; a++){// recursivly call this function on all children
		internalNode->_children.push_back(LoadSkeleton(file, internalNode));
	}
	return internalNode;
}