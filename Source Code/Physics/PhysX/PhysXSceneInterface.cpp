#include "Headers/PhysXSceneInterface.h"

#include "Scenes/Headers/Scene.h"
#include "Graphs/Headers/SceneNode.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

using namespace physx;

namespace Divide {

enum PhysXSceneInterfaceState {
    STATE_LOADING_ACTORS
};

PhysXSceneInterface::PhysXSceneInterface( Scene* parentScene ) : PhysicsSceneInterface( parentScene ),
																 _gScene( nullptr ),
																 _cpuDispatcher( nullptr ) 
{
}

PhysXSceneInterface::~PhysXSceneInterface()
{
	release();
}

bool PhysXSceneInterface::init(){
    physx::PxPhysics* gPhysicsSDK = PhysX::getInstance().getSDK();
    //Create the scene
    if(!gPhysicsSDK){
        ERROR_FN(Locale::get("ERROR_PHYSX_SDK"));
        return false;
    }

    PxSceneDesc sceneDesc(gPhysicsSDK->getTolerancesScale());
    sceneDesc.gravity = PxVec3(DEFAULT_GRAVITY.x, DEFAULT_GRAVITY.y, DEFAULT_GRAVITY.z);
    if(!sceneDesc.cpuDispatcher) {
        _cpuDispatcher = PxDefaultCpuDispatcherCreate(3);
        if(!_cpuDispatcher){
            ERROR_FN(Locale::get("ERROR_PHYSX_INTERFACE_CPU_DISPATCH"));
        }
         sceneDesc.cpuDispatcher = _cpuDispatcher;
    }

	if ( !sceneDesc.filterShader ) {
		sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	}
    sceneDesc.flags |= PxSceneFlag::eENABLE_ACTIVETRANSFORMS;

    _gScene = gPhysicsSDK->createScene(sceneDesc);
    if (!_gScene){
        ERROR_FN(Locale::get("ERROR_PHYSX_INTERFACE_CREATE_SCENE"));
        return false;
    }

    _gScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0);
    _gScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
    PxMaterial* planeMaterial = gPhysicsSDK->createMaterial(0.9f, 0.1f, 1.0f);
    _materials.push_back(planeMaterial);
    PxMaterial* cubeMaterial = gPhysicsSDK->createMaterial(0.1f, 0.4f, 1.0f);
    _materials.push_back(cubeMaterial);
    PxMaterial* sphereMaterial = gPhysicsSDK->createMaterial(0.6f, 0.1f, 0.6f);
    _materials.push_back(sphereMaterial);
    return true;
}

void PhysXSceneInterface::release() {
	if ( !_gScene ) {
		ERROR_FN( Locale::get( "ERROR_PHYSX_CLOSE_INVALID_INTERFACE" ) );
		return;
	}

	D_PRINT_FN( Locale::get( "STOP_PHYSX_SCENE_INTERFACE" ) );

	idle();
	for (PhysXActor* actor : _sceneRigidActors) {
        actor->_actor->release();
	}

    MemoryManager::DELETE_VECTOR(_sceneRigidActors);

	if ( _cpuDispatcher ) {
		_cpuDispatcher->release();
	}
	_gScene->release();
	_gScene = nullptr;
}

void PhysXSceneInterface::idle(){
	if ( _sceneRigidQueue.empty() ) {
		return;
	}
	PhysXActor* crtActor = nullptr;
	while ( _sceneRigidQueue.pop(crtActor) ) {
		assert( crtActor != nullptr );
		_sceneRigidActors.push_back( crtActor );
    }
}

void PhysXSceneInterface::update(const U64 deltaTime){
    if (!_gScene) {
        return;
    }
    SceneGraphNode* newNode = nullptr;
    for(RigidMap::iterator it = _sceneRigidActors.begin(); it != _sceneRigidActors.end(); ++it){
        PhysXActor& crtActor = *(*it);

        if (!crtActor._isInScene) {
            addToScene(crtActor, newNode);
        }
        updateActor(crtActor);
    }
}

namespace {
    PxShape* g_shapes[2048];// = MemoryManager_NEW PxShape*[nShapes];
}
void PhysXSceneInterface::updateActor(PhysXActor& actor){
    STUBBED("ToDo: Add a better synchronization method between SGN's transform and PhysXActor's pose!! -Ionut")
    if(actor.resetTransforms()){

        const vec3<F32>& position = actor.getComponent()->getPosition();
        const vec4<F32>& orientation = actor.getComponent()->getOrientation().asVec4();

        physx::PxTransform posePxTransform(PxVec3(position.x, position.y, position.z),
                                           PxQuat(orientation.x,orientation.y,orientation.z,orientation.w).getConjugate());
        actor._actor->setGlobalPose(posePxTransform);
        actor.resetTransforms(false);

    }else{

        PxU32 nShapes = actor._actor->getNbShapes();
        assert(nShapes < 2048);
        actor._actor->getShapes(g_shapes, nShapes);

        while (nShapes--) {
            updateShape(g_shapes[nShapes], actor);
        }
    }
}

void PhysXSceneInterface::updateShape(PxShape* const shape, PhysXActor& actor){
    if ( !shape ) {
        return;
    }

    PxTransform pT = PxShapeExt::getGlobalPose(*shape, *actor._actor);
    PxQuat pQ = pT.q.getConjugate();

    if(actor._type == physx::PxGeometryType::ePLANE)
        std::swap(pQ.x,pQ.z);
       
    actor.getComponent()->setRotation( Quaternion<F32>( pQ.x, pQ.y, pQ.z, pQ.w ) );
    actor.getComponent()->setPosition( vec3<F32>( pT.p.x, pT.p.y, pT.p.z ) );
}

void PhysXSceneInterface::process(const U64 deltaTime){
    if(!_gScene) return;

    _gScene->simulate(static_cast<physx::PxReal>(getUsToMs(deltaTime)));

    while(!_gScene->fetchResults()) idle();
}

void PhysXSceneInterface::addRigidActor(PhysXActor* const actor, bool threaded) {
	assert( actor != nullptr );

    if (threaded) {
		while ( !_sceneRigidQueue.push( actor ) ) {
			idle();
		}
    } else {
        _sceneRigidActors.push_back(actor);
    }
}

PhysXActor* PhysXSceneInterface::getOrCreateRigidActor(const stringImpl& actorName) {
	if ( !_gScene ) {
		return nullptr;
	}

    for(RigidMap::iterator it = _sceneRigidActors.begin(); it != _sceneRigidActors.end(); ++it) {
		if ( ( *it )->_actorName.compare( actorName ) == 0 ) {
			return ( *it );
		}
    }

    PhysXActor* newActor = MemoryManager_NEW PhysXActor();
    newActor->_actorName = actorName;
    return newActor;
}

void PhysXSceneInterface::addToScene(PhysXActor& actor, SceneGraphNode* outNode){
    STUBBED("ToDo: Only 1 shape per actor for now. Fix This! -Ionut")
    SceneNode* sceneNode = nullptr;

    _gScene->addActor(*(actor._actor));
    U32 nbActors = _gScene->getNbActors(PxActorTypeSelectionFlag::eRIGID_DYNAMIC |
                                        PxActorTypeSelectionFlag::eRIGID_STATIC);
    PxShape** shapes = MemoryManager_NEW PxShape*[actor._actor->getNbShapes()];
    actor._actor->getShapes(shapes, actor._actor->getNbShapes());

    stringImpl sgnName = "";
    bool shadowState = true;
    switch(actor._type){
        case PxGeometryType::eBOX: {
            ResourceDescriptor boxDescriptor("BoxActor");
            sceneNode = CreateResource<Box3D>(boxDescriptor);

            sgnName = "BoxActor_" + Util::toString(nbActors);

            if(sceneNode->GetRef() == 1){
                Material* boxMaterial = CreateResource<Material>(ResourceDescriptor("BoxActor_material"));
                boxMaterial->setDiffuse(vec4<F32>(0.0f,0.0f,1.0f,1.0f));
                boxMaterial->setAmbient(vec4<F32>(0.0f,0.0f,1.0f,1.0f));
                boxMaterial->setEmissive(vec4<F32>(0.1f,0.1f,0.1f,1.0f));
                boxMaterial->setShininess(2);
                boxMaterial->setShadingMode(Material::SHADING_BLINN_PHONG);
                sceneNode->setMaterialTpl(boxMaterial);
            }
        }
        break;

        case PxGeometryType::ePLANE: {
            sgnName = "PlaneActor";
            if(FindResourceImpl<Quad3D>(sgnName)) {
                outNode = GET_ACTIVE_SCENEGRAPH()->findNode(sgnName);
                PhysicsSceneInterface::addToScene(actor, outNode);
                return;
            }
            ResourceDescriptor planeDescriptor(sgnName);
            planeDescriptor.setFlag(true);

            sceneNode = CreateResource<Quad3D>(planeDescriptor);
            Material* planeMaterial = CreateResource<Material>(ResourceDescriptor(sgnName + "_material"));
            planeMaterial->setDiffuse(vec4<F32>(0.4f,0.1f,0.1f,1.0f));
            planeMaterial->setAmbient(vec4<F32>(0.4f,0.1f,0.1f,1.0f));
            planeMaterial->setEmissive(vec4<F32>(0.3f,0.3f,0.3f,1.0f));
            planeMaterial->setShininess(0);
            planeMaterial->setShadingMode(Material::SHADING_BLINN_PHONG);
            shadowState = false;
            sceneNode->setMaterialTpl(planeMaterial);
        }break;
    }

    MemoryManager::DELETE_ARRAY( shapes );

    if(actor._type != PxGeometryType::eTRIANGLEMESH) {
        if(sceneNode){
            sceneNode->renderState().setDrawState(true);
            outNode = _parentScene->getSceneGraph()->addNode(sceneNode, sgnName);
            outNode->getComponent<RenderingComponent>()->castsShadows(shadowState);
        }
        if(!outNode){
            ERROR_FN(Locale::get("ERROR_ACTOR_CAST"), sgnName.c_str());
        }else{
            actor._actorName = sgnName;
            actor.getComponent()->setScale(actor._userData);
        }
    }

    actor._isInScene = true;

    PhysicsSceneInterface::addToScene(actor, outNode);
}

};