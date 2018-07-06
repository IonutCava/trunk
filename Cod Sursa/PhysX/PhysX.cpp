#include "resource.h"
#include "PhysX.h"
#include "pxDecorator.h"
#include "Debug/ErrorStream.h"
#include "Utility/Headers/Guardian.h"
#include "Managers/TextureManager.h"
#include "Hardware/Video/GFXDevice.h"

PhysX::PhysX()
{
	//Initializam toti pointerii cu NULL 
	gPhysicsSDK = NULL;
	gScene = NULL;
	gSceneHW = NULL;
	currentActor = NULL;
	groundPlane = NULL;
	//errorCode-ul se anuleaza initial
	errorCode = NXCE_NO_ERROR;
	//nu dorim afisarea initiala folosind Wireframes
	bDebugWireframeMode = false;
    writeLock = false;
    //initializam cu 0 numarul total de obiecte importate
	
	defaultPosition.set(NxVec3(0,0,0));
	scale = 1.0f;
	groundPos = -220;
	speedMult = 1;
}

void PhysX::setActorDefaultPos(vec3 position)
{
	defaultPosition = (NxVec3)position;
}

void PhysX::setParameters(F32 gravity,bool ShowErrors,F32 scale)
{

	//setam gravitatia in functie de parametrii dati de constructor
	gDefaultGravity.set(0,gravity,0);
	//Daca initializam clasa PhysX cu "true" ca al 2-lea parametru,atunci vom afisa si erorile date de SDK
	mErrorReport = ShowErrors;
    //Scare la care lucram este data de asemenea de constructor
	this->scale = scale;
}

NxReal PhysX::UpdateTime()
{
    NxReal deltaTime;
#ifndef LINUX
    static __int64 gTime,gLastTime;
    __int64 freq;
    QueryPerformanceCounter((LARGE_INTEGER *)&gTime);  // Get current count
    QueryPerformanceFrequency((LARGE_INTEGER *)&freq); // Get processor freq
    deltaTime = (NxReal)(gTime - gLastTime)/(NxReal)freq;
#else
    static clock_t gTime, gLastTime;
    gTime = clock();
    deltaTime = (NxReal)((D32)(gTime - gLastTime) / 1000000.0);
#endif
    gLastTime = gTime;
    return deltaTime *4;
}

bool PhysX::InitNx()
{
	if (gPhysicsSDK) return false; //Daca am initializat SDK-ul deja, renuntam la initializare
	if (mErrorReport)//Ori afisam erori ori ... nu. In functie de asta initializam SDK-ul cu parametri diferiti.
		gPhysicsSDK = NxCreatePhysicsSDK(NX_PHYSICS_SDK_VERSION, NULL, new ErrorStream(), desc, &errorCode);
	else
		gPhysicsSDK = NxCreatePhysicsSDK(NX_PHYSICS_SDK_VERSION);

	if(gPhysicsSDK == NULL) 
	{
		//E posibil sa nu gasim driverul necesar instalat, asa ca anuntam utilizatorul
		Con::getInstance().errorfn("It would seem that you do not have PhysX driver installed!");
		Con::getInstance().printfn("Please visit http://www.nividia.com/object/physx_new.html for more details!");
		return false;
	}

	gPhysicsSDK->setParameter(NX_SKIN_WIDTH, 0.015f*(1.0f/scale)); //Cu cat sa fie invelisul actorului mai mare decat invelisul meshei
	gPhysicsSDK->setParameter(NX_DEFAULT_SLEEP_LIN_VEL_SQUARED, 0.15f*0.15f);
	gPhysicsSDK->setParameter(NX_BOUNCE_THRESHOLD, -20*(1.0f/scale));
    gPhysicsSDK->setParameter(NX_VISUALIZATION_SCALE, 0.5f*(1.0f/scale));
    gPhysicsSDK->setParameter(NX_VISUALIZE_COLLISION_SHAPES, 0);
    gPhysicsSDK->setParameter(NX_VISUALIZE_ACTOR_AXES, 0);
    //Initializam gravitatia scenei
	sceneDesc.gravity		= gDefaultGravity;

	//Initializare multithreading
	sceneDesc.flags |= NX_SF_ENABLE_MULTITHREAD | NX_SF_DISABLE_SCENE_MUTEX;
	sceneDesc.threadMask = 0xfffffffe;
    sceneDesc.internalThreadCount = 2;

    //Momentan folosim suport software deoarece sansele de a gasi un PPU sunt slabe
	//if(!useHardware)
        sceneDesc.simType = NX_SIMULATION_SW; 
	//else
	    //sceneDesc.simType = NX_SIMULATION_HW
	
	gScene = gPhysicsSDK->createScene(sceneDesc);
	//Dupa setarea tuturor parametrilor, putem crea scena ...
	if(gScene == NULL) 
	{
		//... dar trebuie sa verificam si daca crearea acesteia a avut succes
		Con::getInstance().errorfn("Unable to create a PhysX scene!");
		return false;
	}

	// Toti actorii nostri vor folosi acelasi material momentan
	// ToDo: Creare materiale diferite pentru: piatra,pamant,hartie,lemn
	NxMaterial* defaultMaterial = gScene->getMaterialFromIndex(0);
	NxMaterial* rockMaterial = gScene->getMaterialFromIndex(1); 
	NxMaterial* groundMaterial = gScene->getMaterialFromIndex(2);
	NxMaterial* papperMaterial = gScene->getMaterialFromIndex(3);
	NxMaterial* woodMaterial = gScene->getMaterialFromIndex(4);
	//Parametri fizici asociati materialului:
	defaultMaterial->setRestitution(0.0f);
	defaultMaterial->setStaticFriction(0.5f);
	defaultMaterial->setDynamicFriction(0.5f);
    UpdateTime();
	StartPhysics();

	return true;
}

void PhysX::ExitNx()
{
	if(gPhysicsSDK != NULL) // Daca SDK-ul a fost initializat
	{
		//si scena creata
		if(gScene != NULL)
		{
			int nbActors = gScene->getNbActors();
			NxActor** actors = gScene->getActors();
			Con::getInstance().printfn("PhysX: Preparing to delete %d actors", nbActors);
			while (nbActors--)
			{
				NxActor* actor = *actors++;
				RemoveActor(*actor);

			}
			GetPhysicsResults();
			gPhysicsSDK->releaseScene(*gScene); //Renuntam la ea
			gScene = NULL; //O NULL-ificam
		}
		NxReleasePhysicsSDK(gPhysicsSDK); //Inchidem SDK-ul
		gPhysicsSDK = NULL; //Si eliberam si memoria asociata acestuia
	}
}

void PhysX::StartPhysics()
{
	if (!gScene) return;
	gScene->simulate(UpdateTime()*speedMult);
	gScene->flushStream();
}

void PhysX::setSimSpeed(F32 mult)
{
	//iar clamping ...
	if(mult < 0.1f) mult = 0.1f;
	if(mult > 100) mult = 100;
	speedMult = mult;
}

void PhysX::GetPhysicsResults()
{
	if(!gScene) return;
	// Preluam rezultatele de la gScene->simulate(deltaTime)
	while (!gScene->fetchResults(NX_RIGID_BODY_FINISHED, false));
}

void PhysX::ProcessInput()
{ 

}

NxActor* PhysX::AddActor(const NxActorDescBase &actorDesc)
{
	if (!gScene) return NULL;
	while(!gScene->isWritable()){}
	NxActor* actor;
	actor = gScene->createActor(actorDesc);
	if (!actor)	return NULL;
	currentActor = actor;
	return actor;
}

bool PhysX::RemoveActor(NxActor  &actor)
{
	if (!gScene) return false;
	while(!gScene->isWritable()){}
	gScene->releaseActor(actor);
	return true;
}

//Aceasta functie preia o Mesha creata de LoadObj si o converteste intr-un format cunoscut de calculator
//Daca mesha noastra nu exista sau nu contine un vector de fete de tipul:
// Face[i]:   Vert[0].x/.y/.z   Vert[1].x/.y/.z   Vert[2].x/.y/.z
// Face[i+1]: Vert[0].x/.y/.z   Vert[1].x/.y/.z   Vert[2].x/.y/.z
// Face[i+2]: Vert[0].x/.y/.z   Vert[1].x/.y/.z   Vert[2].x/.y/.z
// Fiecare element din din vector trebuie sa contine 3 vertecsi. Fiecare vertex are 3 parametri
// Singura metoda eficienta pentru crearea acestui vector este cu un "struct" de "struct"

bool PhysX::AddShape(DVDFile *mesh,bool convex/*,string cook*/)
{
	/*
	if(mesh == NULL)
	{
		cout << "PhysX error: Couldn't create a PhysX actor because the received mesh was NULL!" << endl;
		return false;
	}
	if(!gScene)
	{
		cout << "PhysX: Error! Could not find a valid scene!" << endl;
		return false;
		
	}
	cout << "Adding actor for current mesh ..." << endl;
    cout << "Started Cooking the following model: "<< endl;
    cout << "*Number of Vertices = " << mesh->vertices.size() << endl;
	cout << "*Number of Faces = " << mesh->triangles.size() << endl;
	cout << "*Point Stride Bytes = " << sizeof(NxU32) << endl;
	cout << "*Triangle Stride Bytes = " << 3*sizeof(NxU32) << endl;

	if (convex)   AddConvexShape(mesh);
	else	AddTriangleShape(mesh);
*/
	return true;
}

bool PhysX::AddConvexShape(DVDFile *mesh)
{
/*	NxVec3 objectLocation = NxVec3(mesh->getPosition());

	NxBuildSmoothNormals(mesh->SimpleFaceArray.size(), mesh->vertices.size(),
		                 (const NxVec3*)&mesh->vertices[0],
						 (const NxU32*)&mesh->SimpleFaceArray[0],
						  NULL,
						  (NxVec3*) &mesh->normals[0],
						  true);

    NxInitCooking();
	NxConvexMesh *mConvexMesh = NULL; 

	PendingActor *Actor = New PositionDecorator(new BodyDecorator
		                                       (new ConvexShapeDecorator
											   (new ConcretePendingActor()
												                              )
																			  )
																			  );
	NxQuat quat;
	NxConvexShapeDesc staticMeshDesc;
	NxConvexMeshDesc   meshDesc;
	meshDesc.numVertices = mesh->vertices.size();
	meshDesc.numTriangles = mesh->triangles.size();
	meshDesc.pointStrideBytes = sizeof(vertex);
	meshDesc.triangleStrideBytes = sizeof(simpletriangle);
	meshDesc.points = &mesh->vertices[0];
	meshDesc.triangles = &mesh->SimpleFaceArray[0];
	meshDesc.flags = 0;

	// cook it
	MemoryWriteBuffer buf;
	NxCookingParams params;
	params.targetPlatform = PLATFORM_PC;
	params.skinWidth = 0.01f*(1.0f/scale);
	params.hintCollisionSpeed = false;
	NxSetCookingParams(params);
	bool status = NxCookConvexMesh(meshDesc,buf);
	if (status)
	{
		
		MemoryReadBuffer readBuffer(buf.data);
        mConvexMesh = this->createConvexMesh(readBuffer);
		staticMeshDesc.localPose.t = NxVec3(0,0,0);
		staticMeshDesc.skinWidth = 0.1f*(1/scale);
		staticMeshDesc.meshData = mConvexMesh;
		Actor->setBody(NULL);
	    Actor->setPos(objectLocation);
	    Actor->setConvexShape(staticMeshDesc);
		Actor->setName(mesh->ModelName);
		NxCloseCooking();
		
	}
	else
	{
		cout << "PhysX error: SetupConvexCollision - COOKING FAILED*****\n";
		NxCloseCooking();
		return true;
	}

    
    gShape = this->AddActor(Actor->createActor());
    if(gShape)
	{
		gShape->userData = (void*)mesh;
		NxVec3 tmp = gShape->getGlobalPosition();
		cout << "PhysX Actor: succesfully cooked convex mesh " << mesh-> ModelName << " at position: x = " << objectLocation.x << " ,y = " << objectLocation.y << " ,z = " << objectLocation.z << "!\n" ;
	}
	else
		cout << "PhysX error: SetupConvexCollision - ACTOR NOT ACTIVE!\n";
	delete Actor;*/
	return true;

}

bool PhysX::AddTriangleShape(DVDFile *mesh)
{
/*	NxVec3 objectLocation = NxVec3(mesh->getPosition());

	NxBuildSmoothNormals(mesh->SimpleFaceArray.size(), mesh->vertices.size(), 
		                 (const NxVec3*)&mesh->vertices[0],
						 (const NxU32*)&mesh->SimpleFaceArray[0],
						  NULL,
						  (NxVec3*)&mesh->normals[0],
						  true);

	NxInitCooking();
	NxTriangleMesh *mTriangleMesh = NULL;
	PendingActor *Actor = New PositionDecorator(new BodyDecorator
		                                       (new TriangleShapeDecorator
											   (new ConcretePendingActor()
												                              )
																			  )
																			  );
	NxQuat quat;
	// Push this actor back into our actor list.
	NxTriangleMeshShapeDesc staticMeshDesc;

	NxTriangleMeshDesc triangleDesc;
	triangleDesc.numVertices            = mesh->vertices.size();
	triangleDesc.numTriangles           = mesh->triangles.size();
	triangleDesc.pointStrideBytes       = sizeof(vertex);
	triangleDesc.triangleStrideBytes	= sizeof(simpletriangle);
	triangleDesc.points			        = &mesh->vertices[0];
	triangleDesc.triangles              = &mesh->SimpleFaceArray[0];
	triangleDesc.flags		     	    = 0;

	// cook it
	MemoryWriteBuffer buf;
	NxCookingParams params;
	params.targetPlatform = PLATFORM_PC;
	params.skinWidth = 0.1f*(1.0f/scale);
	params.hintCollisionSpeed = false;
	NxSetCookingParams(params);
	bool status = NxCookTriangleMesh(triangleDesc, buf);
	if (status)
	{
		MemoryReadBuffer readBuffer(buf.data);
		mTriangleMesh = this->createTriangleMesh(readBuffer);
	    staticMeshDesc.localPose.t = NxVec3(0,0,0);
		staticMeshDesc.skinWidth = 0.1f*(1.0f/scale);
		staticMeshDesc.meshData = mTriangleMesh;
		staticMeshDesc.name = mesh->ModelName.c_str();
	    Actor->setPos(objectLocation);
	    Actor->setBody(NULL);
	    Actor->setTriangleShape(staticMeshDesc);
		Actor->setName(mesh->ModelName);
		NxCloseCooking();
	}
	else
	{
		cout << "PhysX error: SetupTriangleCollision - COOKING FAILED*****\n";
		NxCloseCooking();
		return true;
	}
    gShape = this->AddActor(Actor->createActor());
	if(gShape)
	{
		gShape->userData = (void*)mesh;
        NxVec3 tmp = gShape->getGlobalPosition();
		cout << "PhysX Actor: succesfully cooked triangle mesh " << mesh-> ModelName << " at position: x = " << objectLocation.x << " ,y = " << objectLocation.y << " ,z = " << objectLocation.z << "!\n" ;
	}
	else
		cout << "PhysX error: SetupTriangleCollision - ACTOR NOT ACTIVE!\n";
	delete Actor;
	*/
	return true;
}

NxTriangleMesh* PhysX::createTriangleMesh(const NxStream& stream)
{
	if (gPhysicsSDK)
		return gPhysicsSDK->createTriangleMesh(stream);
	return NULL;
}

NxConvexMesh* PhysX::createConvexMesh(const NxStream& stream)
{
	if (gPhysicsSDK)
		return gPhysicsSDK->createConvexMesh(stream);
	return NULL;
}

void PhysX::UpdateActors()
{
    // Render all the actors in the scene
	if(!gScene) return;
    int nbActors = gScene->getNbActors();
    NxActor** actors = gScene->getActors();
	
    while (nbActors--)
    {
        NxActor* actor = *actors++;
		NxShape *const * shapes = actor->getShapes();
		NxU32 nShapes = actor->getNbShapes();
		
        while (nShapes--)
		{
			NxShape* shape = shapes[nShapes];
			switch(shape->getType())
			{
				case NX_SHAPE_SPHERE:
					this->DrawSphere(shape);
				break;
				case NX_SHAPE_BOX:
					this->DrawBox(shape);
				break;
				case NX_SHAPE_MESH:
					this->DrawObjects(shape);
				break;
				case NX_SHAPE_PLANE:
					this->DrawLowPlane(shape);
				break;
				default:
					Con::getInstance().errorfn( "PhysX error: Invalid shape received for rendering!");
			}
		}
    }
}

void PhysX::DrawObjects(NxShape *obj)
{
	//ToDo:: Update Objects position for each scene static & dynamic mesh arrays. -Ionut
	RenderState s(true,true,true,true);
	GFXDevice::getInstance().setRenderState(s);
	F32 *orient = new F32[16];
	NxMat34 pose = obj->getActor().getGlobalPose();
	pose.M.getColumnMajorStride4(orient);
    //GFXDevice::getInstance().pushMatrix();
	vec3(pose.t.x,pose.t.y,pose.t.z).get(&(orient[12]));
    orient[3] = orient[7] = orient[11] = 0.0f;
    orient[15] = 1.0f;
    //glMultMatrixf(&(orient[0]));
	//GFXDevice::getInstance().renderModel((DVDFile*)obj->getActor().userData);
	//GFXDevice::getInstance().popMatrix();
	delete orient;
}

NxVec3 PhysX::ApplyForceToActor(NxActor* actor, const NxVec3& forceDir, const NxReal forceStrength)
{
	if(actor == NULL) return NxVec3(0,0,0);
	Con::getInstance().printfn("Applying force to actor: [ %s ]", actor->getName());
    NxVec3 forceVec = forceStrength*1000*forceDir;
	actor->addForce(forceVec);
    return forceVec;
}

void PhysX::ResetNx()
{
    ExitNx();
    InitNx();
}

void PhysX::setGroundPlane()
{
	if(!gScene) return;
	if (groundPos > 25000 || groundPos < -25000) groundPos = -220;//clamp? Desi nu e necesar sa introducem o anumita limita
    // Cream un GroundPlane pentru a evita caderea actorilor la infinit
	NxPlaneShapeDesc planeDesc;
	NxActorDesc actorDesc;
	planeDesc.d = groundPos*(1.0f/scale); 
	actorDesc.shapes.pushBack(&planeDesc);
	actorDesc.globalPose.t = NxVec3(0,groundPos*(1.0f/scale),0);
	groundPlane = gScene->createActor(actorDesc);
}