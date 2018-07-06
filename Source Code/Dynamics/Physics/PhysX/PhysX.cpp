#include "Headers/PhysX.h"
#include "Headers/PhysXSceneInterface.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Hardware/Video/Buffers/VertexBufferObject/Headers/VertexBufferObject.h"

#include <Samples/PxToolkit/include/PxToolkit.h>
#include <Samples/PxToolkit/include/PxTkNamespaceMangle.h>
#include <PsFile.h>

using namespace physx;

physx::PxDefaultAllocator     PhysX::_gDefaultAllocatorCallback;
physx::PxDefaultErrorCallback PhysX::_gDefaultErrorCallback;

PhysX::PhysX() : _gPhysicsSDK(NULL),
                 _foundation(NULL),
                 _zoneManager(NULL),
                 _cooking(NULL),
                 _pvdConnection(NULL),
                 _targetScene(NULL){}

I8 PhysX::initPhysics(U8 targetFrameRate) {
    PRINT_FN(Locale::get("START_PHYSX_API"));

    // create foundation object with default error and allocator callbacks.
    _foundation = PxCreateFoundation(PX_PHYSICS_VERSION, _gDefaultAllocatorCallback, _gDefaultErrorCallback);
    assert(_foundation != NULL);

    bool recordMemoryAllocations = true;
    _zoneManager = &PxProfileZoneManager::createProfileZoneManager(_foundation);
    assert(_zoneManager != NULL);

    _gPhysicsSDK = PxCreatePhysics(PX_PHYSICS_VERSION,
                                   *_foundation,
                                   PxTolerancesScale(),
                                   recordMemoryAllocations,
                                   _zoneManager);
   if(_gPhysicsSDK == NULL) {
       ERROR_FN(Locale::get("ERROR_START_PHYSX_API"));
       return PHYSX_INIT_ERROR;
   }

   if(!PxInitExtensions(*_gPhysicsSDK)){
       ERROR_FN(Locale::get("ERROR_EXTENSION_PHYSX_API"));
       return PHYSX_EXTENSION_ERROR;
   }

#ifdef _DEBUG
   _pvdConnection = _gPhysicsSDK->getPvdConnectionManager();
   if(_pvdConnection != NULL){
       if(PxVisualDebuggerExt::createConnection(_pvdConnection,"localhost",5425, 10000) != NULL){
            D_PRINT_FN(Locale::get("CONNECT_PVD_OK"));
       }
   }
#endif
   updateTimeStep(targetFrameRate);
   PRINT_FN(Locale::get("START_PHYSX_API_OK"));
   return NO_ERR;
}

bool PhysX::exitPhysics() {
    if(!_gPhysicsSDK)
        return false;
        
    PRINT_FN(Locale::get("STOP_PHYSX_API"));
    PxCloseExtensions();
    _gPhysicsSDK->release();
    _foundation->release();
    return true;
}

///Process results
void PhysX::process(){
    if(_targetScene){
        _targetScene->process(_timeStep);
    }
}

///Update actors
void PhysX::update(){
    if(_targetScene){
        _targetScene->update();
    }
}

void PhysX::idle(){
}

PhysicsSceneInterface* PhysX::NewSceneInterface(Scene* scene) {
    return New PhysXSceneInterface(scene);
}

void PhysX::initScene(){
    assert(_targetScene);
    _targetScene->init();
}

bool PhysX::createActor(SceneGraphNode* const node, PhysicsActorMask mask, PhysicsCollisionGroup group){
    assert(node != NULL);
    assert(_targetScene != NULL);
    return true;

    //Load cached version from file first
    std::string nodeName("collisionMeshes/node_[_" + node->getName() + "_]");
    nodeName.append(".cm");
    FILE* fp = fopen(nodeName.c_str(), "rb");
    bool fileExists = (fp != NULL);
   
    if(!fileExists){
        VertexBufferObject* nodeVBO = node->getNode<Object3D>()->getGeometryVBO();
        
        PxTriangleMeshDesc meshDesc;
        meshDesc.points.count           = nodeVBO->getPosition().size();
        meshDesc.points.stride          = sizeof(vec3<F32>);
        meshDesc.points.data            = &nodeVBO->getPosition()[0];
        meshDesc.triangles.count        = nodeVBO->getTriangles().size();
        meshDesc.triangles.stride       = 3*sizeof(U32);
        meshDesc.triangles.data         = &nodeVBO->getTriangles()[0];

        if(!_cooking){
            PxCookingParams cookparams;
            cookparams.targetPlatform = PxPlatform::ePC;
            _cooking = PxCreateCooking(PX_PHYSICS_VERSION, *_foundation, cookparams);
        }

        PxToolkit::FileOutputStream stream(nodeName.c_str());

        bool status = _cooking->cookTriangleMesh(meshDesc, stream);
        if(!status){
            ERROR_FN(Locale::get("ERROR_COOK_TRIANGLE_MESH"));
            return false;
        }
    }else{
        fclose(fp);
    }
   
    PxToolkit::FileInputData stream(nodeName.c_str());
    physx::PxTriangleMesh* triangleMesh = _gPhysicsSDK->createTriangleMesh(stream);
    if(!triangleMesh){
        ERROR_FN(Locale::get("ERROR_CREATE_TRIANGLE_MESH"));
        return false;
    }
    const vec3<F32>& scale = node->getTransform()->getScale();
    const vec3<F32>& position = node->getTransform()->getPosition();
    const vec4<F32>& orientation = node->getTransform()->getGlobalOrientation().asVec4();
    PxMeshScale pxScale(PxVec3(scale.x,scale.y,scale.z), PxQuat());
    physx::PxTransform posePxTransform(PxVec3(position.x, position.y, position.z), PxQuat(orientation.x,orientation.y,orientation.z,orientation.w));
    physx::PxTriangleMeshGeometry* geometry = New PxTriangleMeshGeometry(triangleMesh, pxScale);
    physx::PxMaterial* material = _gPhysicsSDK->createMaterial(0.7f, 0.7f, 1.0f);

    switch(mask){
        default:
        case MASK_RIGID_STATIC:{
            physx::PxRigidStatic* rigidActor = _gPhysicsSDK->createRigidStatic(posePxTransform);  
            rigidActor->createShape(*geometry, *material);
            dynamic_cast<PhysXSceneInterface* >(_targetScene)->addRigidStaticActor(rigidActor);
            }break;
        case MASK_RIGID_DYNAMIC:{
            physx::PxRigidDynamic* rigidActor = _gPhysicsSDK->createRigidDynamic(posePxTransform);  
            rigidActor->createShape(*geometry,*material);
            dynamic_cast<PhysXSceneInterface* >(_targetScene)->addRigidDynamicActor(rigidActor);
            }break;
    };

    if(fileExists)
        PRINT_FN(Locale::get("COLLISION_MESH_LOADED_FROM_FILE"), nodeName.c_str());
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