#include "stdafx.h"

#include "Headers/Object3D.h"

#include "Core/Headers/PlatformContext.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"
#include "Physics/Headers/PXDevice.h"
#include "Platform/Video/Headers/RenderPackage.h"

#include "ECS/Components/Headers/AnimationComponent.h"

namespace Divide {

Object3D::Object3D(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, ObjectType type, ObjectFlag flag)
    : Object3D(context, parentCache, descriptorHash, name, name, "", type, to_U32(flag))
{
}

Object3D::Object3D(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, ObjectType type, U32 flagMask)
    : Object3D(context, parentCache, descriptorHash, name, name, "", type, flagMask)
{
}

Object3D::Object3D(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, const stringImpl& resourceName, const stringImpl& resourceLocation, ObjectType type, ObjectFlag flag)
    : Object3D(context, parentCache, descriptorHash, name, resourceName, resourceLocation, type, to_U32(flag))
{
}

Object3D::Object3D(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, const stringImpl& resourceName, const stringImpl& resourceLocation, ObjectType type, U32 flagMask)
    : SceneNode(parentCache, descriptorHash, name, resourceName, resourceLocation, SceneNodeType::TYPE_OBJECT3D),
    _context(context),
    _geometryDirty(true),
    _buffer(nullptr),
    _geometryType(type),
    _geometryFlagMask(flagMask),
    _geometryPartitionID(0U)
{
    if (!getObjectFlag(ObjectFlag::OBJECT_FLAG_NO_VB)) {
        _buffer = context.newVB();
    }

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
}

bool Object3D::isPrimitive() const {
    return _geometryType._value == ObjectType::BOX_3D ||
           _geometryType._value == ObjectType::QUAD_3D ||
           _geometryType._value == ObjectType::PATCH_3D ||
           _geometryType._value == ObjectType::SPHERE_3D;
}

void Object3D::postLoad(SceneGraphNode& sgn) {
     computeTriangleList();
     SceneNode::postLoad(sgn);
}

void Object3D::rebuildVB() {
}

void Object3D::rebuild() {
    if (_geometryDirty) {
        rebuildVB();
        computeTriangleList();
        _geometryDirty = false;
    }
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

bool Object3D::onRender(SceneGraphNode& sgn,
                        const SceneRenderState& sceneRenderState,
                        RenderStagePass renderStagePass) {
    rebuild();
    return SceneNode::onRender(sgn, sceneRenderState, renderStagePass);
}

void Object3D::buildDrawCommands(SceneGraphNode& sgn,
                                 RenderStagePass renderStagePass,
                                 RenderPackage& pkgInOut) {
    if (pkgInOut.drawCommandCount() == 0) {
        GenericDrawCommand cmd;
        VertexBuffer* const vb = getGeometryVB();
        cmd._sourceBuffer = vb;
        cmd._bufferIndex = renderStagePass.index();
        cmd._cmd.indexCount = to_U32(vb->getIndexCount());
        
        GFX::DrawCommand drawCommand;
        drawCommand._drawCommands.push_back(cmd);
        pkgInOut.addDrawCommand(drawCommand);
    }

    SceneNode::buildDrawCommands(sgn, renderStagePass, pkgInOut);
}

// Create a list of triangles from the vertices + indices lists based on primitive type
bool Object3D::computeTriangleList() {
    if (!_geometryTriangles.empty()) {
        return true;
    }

    VertexBuffer* geometry = getGeometryVB();

    DIVIDE_ASSERT(geometry != nullptr,
                  "Object3D error: Please specify a valid VertexBuffer before "
                  "calculating the triangle list!");

    U32 partitionOffset = geometry->getPartitionOffset(_geometryPartitionID);
    U32 partitionCount = geometry->getPartitionIndexCount(_geometryPartitionID);
    PrimitiveType type = (_geometryType._value == ObjectType::MESH ||
                          _geometryType._value == ObjectType::SUBMESH
                              ? PrimitiveType::TRIANGLES
                              : PrimitiveType::TRIANGLE_STRIP);
    // We can't have a VB without vertex positions
    DIVIDE_ASSERT(!geometry->getVertices().empty(),
                  "Object3D error: computeTriangleList called with no position "
                  "data available!");

    if (!_geometryTriangles.empty()) {
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
            const vectorBest<U32>& indices = geometry->getIndices<U32>();
            for (U32 i = indiceStart; i < indiceEnd; i++) {
                curTriangle.set(indices[i - 2], indices[i - 1], indices[i]);
                // Check for correct winding
                if (i % 2 != 0)
                    std::swap(curTriangle.y, curTriangle.z);
                _geometryTriangles.push_back(curTriangle);
            }
        } else {
            const vectorBest<U16>& indices = geometry->getIndices<U16>();
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
            const vectorBest<U32>& indices = geometry->getIndices<U32>();
            for (U32 i = 0; i < indiceCount; i++) {
                _geometryTriangles.push_back(vec3<U32>(indices[i * 3 + 0],
                                                       indices[i * 3 + 1],
                                                       indices[i * 3 + 2]));
            }
        } else {
            const vectorBest<U16>& indices = geometry->getIndices<U16>();
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

vector<SceneGraphNode*> Object3D::filterByType(const vector<SceneGraphNode*>& nodes, ObjectType filter) {
    vector<SceneGraphNode*> result;
    result.reserve(nodes.size());

    for (SceneGraphNode* ptr : nodes) {
        if (ptr && ptr->getNode<Object3D>()->getObjectType() == filter) {
            result.push_back(ptr);
        }
    };

    return result;
}

vectorEASTL<SceneGraphNode*> Object3D::filterByType(const vectorEASTL<SceneGraphNode*>& nodes, ObjectType filter) {
    vectorEASTL<SceneGraphNode*> result;
    result.reserve(nodes.size());

    for (SceneGraphNode* ptr : nodes) {
        if (ptr && ptr->getNode<Object3D>()->getObjectType() == filter) {
            result.push_back(ptr);
        }
    };

    return result;
}

void Object3D::playAnimations(const SceneGraphNode& sgn, const bool state) {
    if (getObjectFlag(ObjectFlag::OBJECT_FLAG_SKINNED)) {
        AnimationComponent* animComp = sgn.get<AnimationComponent>();
        if (animComp != nullptr) {
            animComp->playAnimations(state);
        }
        sgn.forEachChild([state](const SceneGraphNode& child) {
            AnimationComponent* animComp = child.get<AnimationComponent>();
            // Not all submeshes are necessarily animated. (e.g. flag on the back of a character)
            if (animComp != nullptr) {
                animComp->playAnimations(state && animComp->playAnimations());
            }
        });
    }
}

};