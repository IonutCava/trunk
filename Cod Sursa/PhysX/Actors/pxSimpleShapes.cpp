#include "PhysX/PhysX.h"
#include "Rendering/Frustum.h"
#include "Utility/Headers/ParamHandler.h"
#include "Hardware/Video/GFXDevice.h"
#include "Geometry/Predefined/Sphere3D.h"
#include "Geometry/Predefined/Quad3D.h"
#include "Geometry/Predefined/Box3D.h"

void PhysX::CreateCube(I16 size=2)
{
	CreateCube(defaultPosition,size);
}

void PhysX::CreateCube(NxVec3 position,I16 size=2)
{
	if(!gScene) return;
	const NxVec3* initial_velocity=NULL;
	vec3 v1(-(F32)size/2.0f,-(F32)size/2.0f,(F32)size/2.0f);
	vec3 v2((F32)size/2.0f,(F32)size/2.0f,-(F32)size/2.0f);
	// Create body
	NxBodyDesc BodyDesc;
	BodyDesc.angularDamping	= 0.5f;
	if(initial_velocity) BodyDesc.linearVelocity = *initial_velocity;

	NxBoxShapeDesc BoxDesc;
	BoxDesc.dimensions		= NxVec3(F32(size), F32(size), F32(size));

	NxActorDesc ActorDesc;
	ActorDesc.shapes.pushBack(&BoxDesc);
	ActorDesc.body			= &BodyDesc;
	ActorDesc.density		= 100.0f;
	ActorDesc.globalPose.t  = position;
	while(!gScene->isWritable()){}
	NxActor* newActor = gScene->createActor(ActorDesc);
	
	if(newActor)
	{
		
		BoundingBox *cube = new BoundingBox(v1,v2);
		newActor->userData = (void*)cube;
	}
	else
	{
		gScene->flushStream();
		CreateCube(position,size);
	}
}

void PhysX::CreateStack(I16 size)
{
	if(!gScene) return;
	F32 CubeSize = 2.0f;
	F32 Spacing = 0.0001f;
	NxVec3 Pos(defaultPosition.x,defaultPosition.y + CubeSize,defaultPosition.z);
	F32 Offset = -size * (CubeSize * 2.0f + Spacing) * 0.5f + defaultPosition.x;
	while(size)
	{
		for(I16 i=0;i<size;i++)
		{
			Pos.x = Offset + i * (CubeSize * 2.0f + Spacing);
			CreateCube(Pos,(I16)CubeSize);
		}
		Offset += CubeSize;
		Pos.y += (CubeSize * 2.0f + Spacing);
		size--;
	}
}

void PhysX::CreateTower(I16 size)
{
	if(!gScene) return;
	F32 CubeSize = 2.0f;
	F32 Spacing = 0.01f;
	NxVec3 Pos(defaultPosition.x,defaultPosition.y + CubeSize,defaultPosition.z);
	while(size)
	{
		CreateCube(Pos,(I16)CubeSize);
		Pos.y += (CubeSize * 2.0f + Spacing);
		size--;
	}
}

void PhysX::DrawLowPlane(NxShape *plane)
{
	NxMat34 pose = plane->getActor().getGlobalPose();
	F32 *orient = new F32[16];
	pose.M.getColumnMajorStride4(orient);
	vec3(pose.t.x,pose.t.y,pose.t.z).get(&(orient[12]));
    orient[3] = orient[7] = orient[11] = 0.0f;
    orient[15] = 1.0f;
	D32 zFar = ParamHandler::getInstance().getParam<D32>("zFar");
	Quad3D quad;
	quad.getTransform()->scale(vec3(2.0f*zFar,0,2.0f*zFar));
    quad.getTransform()->setPosition(vec3(pose.t.x,pose.t.y,pose.t.z));
	GFXDevice::getInstance().drawQuad3D(&quad);
	quad.unload();
	delete orient;
	orient = NULL;
}

void PhysX::DrawSphere(NxShape *sphere)
{
	NxMat34 pose = sphere->getActor().getGlobalPose();
	NxReal r = sphere->isSphere()->getRadius();
	F32 *orient = new F32[16];
	pose.M.getColumnMajorStride4(orient);
	NxQuat temp(pose.M);
    orient[3] = orient[7] = orient[11] = 0.0f;  orient[15] = 1.0f;
	Sphere3D visualSphere(1,9);
	visualSphere.getTransform()->setTransforms(mat4(orient));
	GFXDevice::getInstance().drawSphere3D(&visualSphere);
	visualSphere.unload();
	delete orient;
	orient = NULL;
}

void PhysX::DrawBox(NxShape *box)
{
	NxMat34 pose = box->getActor().getGlobalPose();
	F32 *orient = new F32[16];
	GFXDevice::getInstance().setColor(vec3(0.3f,0.3f,0.8f));
	BoundingBox *cube = (BoundingBox*)box->getActor().userData;
	pose.M.getColumnMajorStride4(orient);
	NxQuat temp(pose.M);
    orient[3] = orient[7] = orient[11] = 0.0f; orient[15] = 1.0f;
	Box3D visualCube((cube->getMin()).distance( cube->getExtent()));
	visualCube.getMaterial().diffuse = vec3(0.3f,0.3f,0.3f);
	visualCube.getTransform()->setTransforms(mat4(orient));
	GFXDevice::getInstance().drawBox3D(&visualCube);
	visualCube.unload();
	delete orient;
	orient = NULL;
}

void PhysX::CreateSphere(I16 size = 2)
{
	if(!gScene) return;
	const NxVec3* initial_velocity=NULL;
	// Create body
	NxBodyDesc BodyDesc;
	NxActorDesc ActorDesc;
	NxSphereShapeDesc SphereDesc;
	if(initial_velocity) BodyDesc.linearVelocity = *initial_velocity;
	
	SphereDesc.radius = (NxReal)size;
	ActorDesc.shapes.pushBack(&SphereDesc);
	ActorDesc.body			= &BodyDesc;
	ActorDesc.density		= 100.0f;
	ActorDesc.globalPose.t  = defaultPosition;
	NxActor* newActor = gScene->createActor(ActorDesc);
	if(newActor)
		newActor->userData = (void*)size;
	else
	{
		gScene->flushStream();
		CreateSphere(size);
	}
}
