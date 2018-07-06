#include "Headers/Object3D.h"

#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"
#include "Physics/Headers/PXDevice.h"

namespace Divide {

Object3D::Object3D(const stringImpl& name, ObjectType type, ObjectFlag flag)
    : Object3D(name, "", type, to_uint(flag))
{
}

Object3D::Object3D(const stringImpl& name, ObjectType type, U32 flagMask)
    : Object3D(name, "", type, flagMask)
{
}

Object3D::Object3D(const stringImpl& name, const stringImpl& resourceLocation, ObjectType type, ObjectFlag flag)
    : Object3D(name, resourceLocation, type, to_uint(flag))
{
}

Object3D::Object3D(const stringImpl& name, const stringImpl& resourceLocation, ObjectType type, U32 flagMask)
    : SceneNode(name, resourceLocation, SceneNodeType::TYPE_OBJECT3D),
    _update(false),
    _playAnimations(true),
    _geometryType(type),
    _geometryFlagMask(flagMask),
    _geometryPartitionID(0U)
{
    _buffer =
        BitCompare(_geometryFlagMask, to_const_uint(ObjectFlag::OBJECT_FLAG_NO_VB))
        ? nullptr
        : GFX_DEVICE.newVB();

    switch (type) {
        case ObjectType::BOX_3D :
            _rigidBodyShape = RigidBodyShape::SHAPE_BOX;
            break;
        case ObjectType::QUAD_3D :
            _rigidBodyShape = RigidBodyShape::SHAPE_PLANE;
            break;
        case ObjectType::SPHERE_3D :
            _rigidBodyShape = RigidBodyShape::SHAPE_SPHERE;
            break;
        case ObjectType::TERRAIN :
            _rigidBodyShape = RigidBodyShape::SHAPE_HEIGHTFIELD;
            break;
        case ObjectType::TEXT_3D :
            _rigidBodyShape = RigidBodyShape::SHAPE_TRIANGLEMESH;
        case ObjectType::MESH: {
            STUBBED("ToDo: Add capsule and convex mesh support for 3D Objects! -Ionut");
            //if (true) { // general meshes? Maybe have a concave flag?
                _rigidBodyShape = RigidBodyShape::SHAPE_TRIANGLEMESH;
            /*} else { 
                if (true) { // skinned characters?
                    _rigidBodyShape = RigidBodyShape::SHAPE_CAPSULE;
                } else { // have a convex flag for imported meshes?
                    _rigidBodyShape = RigidBodyShape::SHAPE_CONVEXMESH;
                }
            }*/
            } break;
        default:
            _rigidBodyShape = RigidBodyShape::SHAPE_COUNT;
            break;
    };
}

Object3D::~Object3D()
{
    if (!BitCompare(_geometryFlagMask,
                    to_const_uint(ObjectFlag::OBJECT_FLAG_NO_VB))) {
        MemoryManager::DELETE(_buffer);
    }
}

bool Object3D::isPrimitive() {
    return _geometryType == ObjectType::BOX_3D ||
           _geometryType == ObjectType::QUAD_3D ||
           _geometryType == ObjectType::SPHERE_3D ||
           _geometryType == ObjectType::TEXT_3D;
}

void Object3D::postLoad(SceneGraphNode& sgn) {
     computeTriangleList();
     SceneNode::postLoad(sgn);
}

void Object3D::setGeometryVB(VertexBuffer* const vb) {
    DIVIDE_ASSERT(_buffer == nullptr,
                  "Object3D error: Please remove the previous vertex buffer of "
                  "this Object3D before specifying a new one!");
    _buffer = vb;
}

VertexBuffer* const Object3D::getGeometryVB() const {
    return _buffer;
}

bool Object3D::onRender(SceneGraphNode& sgn, RenderStage currentStage) {
    return onRender(currentStage);
}

bool Object3D::onRender(RenderStage currentStage) {
    return getState() == ResourceState::RES_LOADED;
}

bool Object3D::getDrawCommands(SceneGraphNode& sgn,
                               RenderStage renderStage,
                               const SceneRenderState& sceneRenderState,
                               vectorImpl<GenericDrawCommand>& drawCommandsOut) {

    // If we got to this point without any draw commands, and no early-out, add a default command
    if (isPrimitive()) {
        drawCommandsOut.resize(1);
        GenericDrawCommand& cmd = drawCommandsOut.front();

        RenderingComponent* const renderable = sgn.get<RenderingComponent>();
        VertexBuffer* const vb = getGeometryVB();

        cmd.renderMask(renderable->renderMask());
        cmd.stateHash(renderable->getDrawStateHash(renderStage));
        cmd.shaderProgram(renderable->getDrawShader(renderStage));
        cmd.sourceBuffer(vb);
        cmd.cmd().indexCount = to_uint(vb->getIndexCount());
    }

    return SceneNode::getDrawCommands(sgn, renderStage, sceneRenderState, drawCommandsOut);
}

// Create a list of triangles from the vertices + indices lists based on
// primitive type
bool Object3D::computeTriangleList(bool force) {
    if (!_geometryTriangles.empty() && !force) {
        return true;
    }

    VertexBuffer* geometry = getGeometryVB();

    DIVIDE_ASSERT(geometry != nullptr,
                  "Object3D error: Please specify a valid VertexBuffer before "
                  "calculating the triangle list!");

    U32 partitionOffset = geometry->getPartitionOffset(_geometryPartitionID);
    U32 partitionCount = geometry->getPartitionIndexCount(_geometryPartitionID);
    PrimitiveType type = (_geometryType == ObjectType::MESH ||
                          _geometryType == ObjectType::SUBMESH
                              ? PrimitiveType::TRIANGLES
                              : PrimitiveType::TRIANGLE_STRIP);
    // We can't have a VB without vertex positions
    DIVIDE_ASSERT(!geometry->getVertices().empty(),
                  "Object3D error: computeTriangleList called with no position "
                  "data available!");

    if (!_geometryTriangles.empty() && force) {
        _geometryTriangles.resize(0);
    }

    if (geometry->getIndexCount() == 0) {
        return false;
    }

    U32 indiceCount = partitionCount;
    bool largeIndices = geometry->usesLargeIndices();
    if (type == PrimitiveType::TRIANGLE_STRIP) {
        U32 indiceStart = 2 + partitionOffset;
        U32 indiceEnd = indiceCount + partitionOffset;
        vec3<U32> curTriangle;
        _geometryTriangles.reserve(indiceCount / 2);
        if (largeIndices) {
            const vectorImplAligned<U32>& indices = geometry->getIndices<U32>();
            for (U32 i = indiceStart; i < indiceEnd; i++) {
                curTriangle.set(indices[i - 2], indices[i - 1], indices[i]);
                // Check for correct winding
                if (i % 2 != 0)
                    std::swap(curTriangle.y, curTriangle.z);
                _geometryTriangles.push_back(curTriangle);
            }
        } else {
            const vectorImplAligned<U16>& indices = geometry->getIndices<U16>();
            for (U32 i = indiceStart; i < indiceEnd; i++) {
                curTriangle.set(indices[i - 2], indices[i - 1], indices[i]);
                // Check for correct winding
                if (i % 2 != 0)
                    std::swap(curTriangle.y, curTriangle.z);
                _geometryTriangles.push_back(curTriangle);
            }
        }
    } else if (type == PrimitiveType::TRIANGLES) {
        indiceCount /= 3;
        _geometryTriangles.reserve(indiceCount);
        if (largeIndices) {
            const vectorImplAligned<U32>& indices = geometry->getIndices<U32>();
            for (U32 i = 0; i < indiceCount; i++) {
                _geometryTriangles.push_back(vec3<U32>(indices[i * 3 + 0],
                                                       indices[i * 3 + 1],
                                                       indices[i * 3 + 2]));
            }
        } else {
            const vectorImplAligned<U16>& indices = geometry->getIndices<U16>();
            for (U32 i = 0; i < indiceCount; i++) {
                _geometryTriangles.push_back(vec3<U32>(indices[i * 3 + 0],
                                                       indices[i * 3 + 1],
                                                       indices[i * 3 + 2]));
            }
        }
    }

    // Check for degenerate triangles
    _geometryTriangles.erase(
        std::partition(
            std::begin(_geometryTriangles), std::end(_geometryTriangles),
            [](const vec3<U32>& triangle) -> bool {
                return (triangle.x != triangle.y && triangle.x != triangle.z &&
                        triangle.y != triangle.z);
            }),
        std::end(_geometryTriangles));

    DIVIDE_ASSERT(!_geometryTriangles.empty(),
                  "Object3D error: computeTriangleList() failed to generate "
                  "any triangles!");
    return true;
}
};