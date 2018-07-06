#include "Headers/PhysX.h"
#include "Headers/PhysXSceneInterface.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Geometry/Shapes/Headers/SkinnedMesh.h"
#include "Hardware/Video/Buffers/VertexBufferObject/Headers/VertexBufferObject.h"

#include <Samples/PxToolkit/include/PxToolkit.h>
#include <Samples/PxToolkit/include/PxTkNamespaceMangle.h>
#include <PsFile.h>

using namespace physx;

physx::PxDefaultAllocator     PhysX::_gDefaultAllocatorCallback;
physx::PxDefaultErrorCallback PhysX::_gDefaultErrorCallback;

physx::PxProfileZone* PhysX::getOrCreateProfileZone(PxFoundation& inFoundation) 
{ 	
#ifdef PHYSX_PROFILE_SDK
    if ( _profileZone == NULL )
        _profileZone = &physx::PxProfileZone::createProfileZone( &inFoundation, "SampleProfileZone", gProfileNameProvider );
#endif
    return _profileZone;
}


PhysX::PhysX() : _gPhysicsSDK(NULL),
                 _foundation(NULL),
                 _zoneManager(NULL),
                 _cooking(NULL),
                 _profileZone(NULL),
                 _pvdConnection(NULL),
                 _targetScene(NULL),
                 _accumulator(0.0f),
                 _timeStepFactor(0.0f),
                 _timeStep(0.0f)
{
}

I8 PhysX::initPhysics(U8 targetFrameRate) {
    PRINT_FN(Locale::get("START_PHYSX_API"));

    // create foundation object with default error and allocator callbacks.
    _foundation = PxCreateFoundation(PX_PHYSICS_VERSION, _gDefaultAllocatorCallback, _gDefaultErrorCallback);
    assert(_foundation != NULL);
    
    bool recordMemoryAllocations = false;

#ifdef _DEBUG
    recordMemoryAllocations = true;
    _zoneManager = &PxProfileZoneManager::createProfileZoneManager(_foundation);
    assert(_zoneManager != NULL);
#endif

    _gPhysicsSDK = PxCreatePhysics(PX_PHYSICS_VERSION,
                                   *_foundation,
                                   PxTolerancesScale(),
                                   recordMemoryAllocations,
                                   _zoneManager);
    if(_gPhysicsSDK == NULL) {
        ERROR_FN(Locale::get("ERROR_START_PHYSX_API"));
        return PHYSX_INIT_ERROR;
    }
  
#ifdef _DEBUG
    if(getOrCreateProfileZone(*_foundation))
        _zoneManager->addProfileZone(*_profileZone);
    _pvdConnection = _gPhysicsSDK->getPvdConnectionManager();
    _gPhysicsSDK->getPvdConnectionManager()->addHandler(*this);
    if(_pvdConnection != NULL){
        if(PxVisualDebuggerExt::createConnection(_pvdConnection,"localhost",5425, 10000) != NULL){
             D_PRINT_FN(Locale::get("CONNECT_PVD_OK"));
        }
    }
#endif

    if(!PxInitExtensions(*_gPhysicsSDK)){
        ERROR_FN(Locale::get("ERROR_EXTENSION_PHYSX_API"));
        return PHYSX_EXTENSION_ERROR;
    }
    
    if(!_cooking){
        PxCookingParams cookparams;
        cookparams.targetPlatform = PxPlatform::ePC;
        _cooking = PxCreateCooking(PX_PHYSICS_VERSION, *_foundation, cookparams);
    }

    updateTimeStep(targetFrameRate);
    PRINT_FN(Locale::get("START_PHYSX_API_OK"));
    return NO_ERR;
}

bool PhysX::exitPhysics() {
    if(!_gPhysicsSDK)
        return false;
        
    PRINT_FN(Locale::get("STOP_PHYSX_API"));
    
    if(_cooking)
        _cooking->release();

    PxCloseExtensions();
    _gPhysicsSDK->release();
    _foundation->release();
    return true;
}

inline void PhysX::updateTimeStep(U8 timeStepFactor) {
    CLAMP<U8>(timeStepFactor,1,timeStepFactor);
    _timeStepFactor = timeStepFactor;
    _timeStep = 1.0f / _timeStepFactor;
    _timeStep *= ParamHandler::getInstance().getParam<F32>("simSpeed");
}

///Process results
void PhysX::process(const D32 deltaTime){
    if(_targetScene && _timeStep > EPSILON){
        _accumulator  += deltaTime;
        
        if(_accumulator < _timeStep)
            return;
         
        _accumulator -= _timeStep;
        _targetScene->process(_timeStep);
    }
}

///Update actors
void PhysX::update(const D32 deltaTime){
    if(_targetScene){
        _targetScene->update(deltaTime);
    }
}

void PhysX::idle(){
    if(_targetScene){
        _targetScene->idle();
    }
}

PhysicsSceneInterface* PhysX::NewSceneInterface(Scene* scene) {
    return New PhysXSceneInterface(scene);
}

void PhysX::initScene(){
    assert(_targetScene);
    _targetScene->init();
}

bool PhysX::createActor(SceneGraphNode* const node, const std::string& sceneName, PhysicsActorMask mask, PhysicsCollisionGroup group){
    assert(node != NULL);
    assert(_targetScene != NULL);
    Object3D* sNode = node->getNode<Object3D>();

    //Load cached version from file first
    std::string nodeName("XML/Scenes/" + sceneName + "/collisionMeshes/node_[_" + sNode->getName() + "_]");
    nodeName.append(".cm");

    FILE* fp = fopen(nodeName.c_str(), "rb");
    bool ok = false;
    if(fp)
    {
        fseek(fp, 0, SEEK_END);
        PxU32 filesize=ftell(fp);
        fclose(fp);
        ok = (filesize != 0);
    }

    //if(group == GROUP_NON_COLLIDABLE)
    //   return true;

    bool isSkinnedMesh = sNode->getType() == Object3D::SUBMESH ? 
                         (node->getParent()->getNode<Object3D>()->getFlag() == Object3D::OBJECT_FLAG_SKINNED) : 
                         false;
                       
    //Disabled for now - Ionut
    isSkinnedMesh = false;
    if(!ok){
        VertexBufferObject* nodeVBO = sNode->getGeometryVBO();
        if(nodeVBO->getPosition().empty())
            return true;

        if(nodeVBO->getTriangles().empty()) {
            nodeVBO->computeTriangles(true);
            if(!nodeVBO->computeTriangleList())
                return false;
        }

        PxToolkit::FileOutputStream stream(nodeName.c_str());
        if(!isSkinnedMesh) {
            PxTriangleMeshDesc meshDesc;
            meshDesc.points.count     = nodeVBO->getPosition().size();
            meshDesc.points.stride    = sizeof(vec3<F32>);
            meshDesc.points.data      = &nodeVBO->getPosition()[0];
            meshDesc.triangles.count  = nodeVBO->getTriangles().size();
            meshDesc.triangles.stride = 3*sizeof(U32);
            meshDesc.triangles.data   = &nodeVBO->getTriangles()[0];
            //if(!nodeVBO->usesLargeIndices())
                //meshDesc.flags = PxMeshFlag::e16_BIT_INDICES;

            if(!_cooking->cookTriangleMesh(meshDesc, stream)){
                ERROR_FN(Locale::get("ERROR_COOK_TRIANGLE_MESH"));
                return false;
            }
        }

    }else{
        PRINT_FN(Locale::get("COLLISION_MESH_LOADED_FROM_FILE"), nodeName.c_str());
    }
    
    PhysXSceneInterface* targetScene = dynamic_cast<PhysXSceneInterface* >(_targetScene);
    PhysXActor* tempActor = NULL;
    Transform* nodeTransform = NULL;
    if(sNode->getType() == Object3D::SUBMESH){
        tempActor = targetScene->getOrCreateRigidActor(node->getParent()->getName());
        nodeTransform = node->getParent()->getTransform();
    }else{
        tempActor = targetScene->getOrCreateRigidActor(node->getName());
        nodeTransform = node->getTransform();
    }
    assert(tempActor != NULL);
    assert(nodeTransform != NULL);
    tempActor->_transform = nodeTransform;

    if(!tempActor->_actor) {
        const vec3<F32>& position = nodeTransform->getPosition();
        const vec4<F32>& orientation = nodeTransform->getGlobalOrientation().asVec4();

        physx::PxTransform posePxTransform(PxVec3(position.x, position.y, position.z),
                                           PxQuat(orientation.x,orientation.y,orientation.z,orientation.w).getConjugate());

        if(mask != MASK_RIGID_DYNAMIC)
            tempActor->_actor = _gPhysicsSDK->createRigidStatic(posePxTransform);
        else {
            tempActor->_actor = _gPhysicsSDK->createRigidDynamic(posePxTransform);
            tempActor->_isDynamic = true;
        }
        tempActor->_type = PxGeometryType::eTRIANGLEMESH;
        targetScene->addRigidActor(tempActor, false);
        nodeTransform->cleanPhysics();
    }
    
    physx::PxGeometry* geometry = NULL;
    if(!isSkinnedMesh){
        PxToolkit::FileInputData stream(nodeName.c_str());
        physx::PxTriangleMesh* triangleMesh = _gPhysicsSDK->createTriangleMesh(stream);
        if(!triangleMesh){
            ERROR_FN(Locale::get("ERROR_CREATE_TRIANGLE_MESH"));
            return false;
        }

        const vec3<F32>& scale = nodeTransform->getScale();
        geometry = New PxTriangleMeshGeometry(triangleMesh, 
                                              PxMeshScale(PxVec3(scale.x,scale.y,scale.z),
                                              PxQuat::createIdentity()));
    }else{
       const BoundingBox& maxBB = node->getBoundingBox();
       geometry = New PxBoxGeometry(maxBB.getWidth(),maxBB.getHeight(),maxBB.getDepth());
    }

    assert(geometry != NULL);

    static physx::PxMaterial* material = _gPhysicsSDK->createMaterial(0.7f, 0.7f, 1.0f);

    tempActor->_actor->createShape(*geometry, *material);
    return true;
};

namespace PxToolkit
{

///////////////////////////////////////////////////////////////////////////////

    MemoryOutputStream::MemoryOutputStream() :
        mData		(NULL),
        mSize		(0),
        mCapacity	(0)
    {
    }

    MemoryOutputStream::~MemoryOutputStream()
    {
        if(mData)
            delete[] mData;
    }

    PxU32 MemoryOutputStream::write(const void* src, PxU32 size)
    {
        PxU32 expectedSize = mSize + size;
        if(expectedSize > mCapacity)
        {
            mCapacity = expectedSize + 4096;

            PxU8* newData = new PxU8[mCapacity];
            PX_ASSERT(newData!=NULL);

            if(newData)
            {
                memcpy(newData, mData, mSize);
                delete[] mData;
            }
            mData = newData;
        }
        memcpy(mData+mSize, src, size);
        mSize += size;
        return size;
    }

///////////////////////////////////////////////////////////////////////////////

    MemoryInputData::MemoryInputData(PxU8* data, PxU32 length) :
        mSize	(length),
        mData	(data),
        mPos	(0)
    {
    }

    PxU32 MemoryInputData::read(void* dest, PxU32 count)
    {
        PxU32 length = PxMin<PxU32>(count, mSize-mPos);
        memcpy(dest, mData+mPos, length);
        mPos += length;
        return length;
    }

    PxU32 MemoryInputData::getLength() const
    {
        return mSize;
    }

    void MemoryInputData::seek(PxU32 offset)
    {
        mPos = PxMin<PxU32>(mSize, offset);
    }

    PxU32 MemoryInputData::tell() const
    {
        return mPos;
    }

///////////////////////////////////////////////////////////////////////////////

    FileOutputStream::FileOutputStream(const char* filename)
    {
        mFile = NULL;
        physx::Ps::fopen_s(&mFile, filename, "wb");
    }

    FileOutputStream::~FileOutputStream()
    {
        if(mFile)
            fclose(mFile);
    }

    PxU32 FileOutputStream::write(const void* src, PxU32 count)
    {
        return mFile ? (PxU32)fwrite(src, 1, count, mFile) : 0;
    }

    bool FileOutputStream::isValid()
    {
        return mFile != NULL;
    }

///////////////////////////////////////////////////////////////////////////////

    FileInputData::FileInputData(const char* filename)
    {
        mFile = NULL;
        Ps::fopen_s(&mFile, filename, "rb");

        if(mFile)
        {
            fseek(mFile, 0, SEEK_END);
            mLength = ftell(mFile);
            fseek(mFile, 0, SEEK_SET);
        }
        else
        {
            mLength = 0;
        }
    }

    FileInputData::~FileInputData()
    {
        if(mFile)
            fclose(mFile);
    }

    PxU32 FileInputData::read(void* dest, PxU32 count)
    {
        PX_ASSERT(mFile);
        const size_t size = fread(dest, 1, count, mFile);
        PX_ASSERT(PxU32(size)==count);
        return PxU32(size);
    }

    PxU32 FileInputData::getLength() const
    {
        return mLength;
    }

    void FileInputData::seek(PxU32 pos)
    {
        fseek(mFile, pos, SEEK_SET);
    }

    PxU32 FileInputData::tell() const
    {
        return ftell(mFile);
    }

    bool FileInputData::isValid() const
    {
        return mFile != NULL;
    }

};