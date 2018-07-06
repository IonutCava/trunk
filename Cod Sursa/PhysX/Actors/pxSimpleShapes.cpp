#include "PhysX/PhysX.h"
#include "Rendering/Frustum.h"
#include "Utility/Headers/BoundingBox.h"
#include "Hardware/Video/GFXDevice.h"

void PhysX::CreateCube(int size=2)
{
	CreateCube(defaultPosition,size);
}

void PhysX::CreateCube(NxVec3 position,int size=2)
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
		cube->size = (F32)size;
		newActor->userData = (void*)cube;
	}
	else
	{
		gScene->flushStream();
		CreateCube(position,size);
	}
}


void PhysX::CreateStack(int size)
{
	if(!gScene) return;
	F32 CubeSize = 2.0f;
	F32 Spacing = 0.0001f;
	NxVec3 Pos(defaultPosition.x,defaultPosition.y + CubeSize,defaultPosition.z);
	F32 Offset = -size * (CubeSize * 2.0f + Spacing) * 0.5f + defaultPosition.x;
	while(size)
	{
		for(int i=0;i<size;i++)
		{
			Pos.x = Offset + i * (CubeSize * 2.0f + Spacing);
			CreateCube(Pos,(int)CubeSize);
		}
		Offset += CubeSize;
		Pos.y += (CubeSize * 2.0f + Spacing);
		size--;
	}
}

void PhysX::CreateTower(int size)
{
	if(!gScene) return;
	F32 CubeSize = 2.0f;
	F32 Spacing = 0.01f;
	NxVec3 Pos(defaultPosition.x,defaultPosition.y + CubeSize,defaultPosition.z);
	while(size)
	{
		CreateCube(Pos,(int)CubeSize);
		Pos.y += (CubeSize * 2.0f + Spacing);
		size--;
	}
}

void PhysX::DrawLowPlane(NxShape *plane)
{
	GFXDevice::getInstance().pushMatrix();
	NxMat34 pose = plane->getActor().getGlobalPose();
	F32 *orient = new F32[16];
	pose.M.getColumnMajorStride4(orient);
	vec3(pose.t.x,pose.t.y,pose.t.z).get(&(orient[12]));
    orient[3] = orient[7] = orient[11] = 0.0f;
    orient[15] = 1.0f;
    glMultMatrixf(&(orient[0]));
    glScaled(8192,0,8192);
    GFXDevice::getInstance().popMatrix();
	delete orient;
}

void PhysX::DrawSphere(NxShape *sphere)
{

	glPushAttrib(GL_ALL_ATTRIB_BITS);
	NxMat34 pose = sphere->getActor().getGlobalPose();
	NxReal r = sphere->isSphere()->getRadius();
	GFXDevice::getInstance().pushMatrix();
	F32 *orient = new F32[16];
	pose.M.getColumnMajorStride4(orient);
	vec3(pose.t.x,pose.t.y,pose.t.z).get(&(orient[12]));
    orient[3] = orient[7] = orient[11] = 0.0f;
    orient[15] = 1.0f;
    glMultMatrixf(&(orient[0]));
    glScaled(r,r,r);
    GLUquadricObj * quadObj = gluNewQuadric ();
    gluQuadricDrawStyle (quadObj, GLU_FILL);
    gluQuadricNormals (quadObj, GLU_SMOOTH); 
    gluQuadricOrientation(quadObj,GLU_OUTSIDE);
    gluSphere(quadObj, 1.0f, 9, 7);        //unit sphere
    gluDeleteQuadric(quadObj);
    GFXDevice::getInstance().popMatrix();
	glPopAttrib();
	delete orient;
}

void PhysX::DrawBox(NxShape *box)
{
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	NxMat34 pose = box->getActor().getGlobalPose();
	F32 *orient = new F32[16];
	GFXDevice::getInstance().setColor(0.3f,0.3f,0.8f);
	BoundingBox *cube = (BoundingBox*)box->getActor().userData;
	/*if(!Frustum::getInstance().ContainsBoundingBox(*cube)) return;*/
	GFXDevice::getInstance().pushMatrix();
	pose.M.getColumnMajorStride4(orient);
	//glEnable(GL_COLOR_MATERIAL);
	//F32 color[4] = { 0.3f, 0.3f, 0.3f, 1.0f };
	//glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
	//glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color);
	vec3(pose.t.x,pose.t.y,pose.t.z).get(&(orient[12]));
    orient[3] = orient[7] = orient[11] = 0.0f;
    orient[15] = 1.0f;
    glMultMatrixf(&(orient[0]));
	cube->Translate(vec3(pose.t.x,pose.t.y,pose.t.z));
	glutSolidCube(cube->size*2.0f);
	//glDisable(GL_COLOR_MATERIAL);
	GFXDevice::getInstance().popMatrix();
	GFXDevice::getInstance().setColor(1.0f,1.0f,1.0f);
    glPopAttrib();	
	delete orient;
}

void PhysX::CreateSphere(int size = 2)
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