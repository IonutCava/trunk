#include "stdafx.h"

#include "Headers/Object3D.h"

#include "Geometry/Material/Headers/Material.h"
#include "Managers/Headers/SceneManager.h"
#include "Physics/Headers/PXDevice.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderPackage.h"

#include "ECS/Components/Headers/AnimationComponent.h"

namespace Divide {

Object3D::Object3D(GFXDevice& context, ResourceCache* parentCache, size_t descriptorHash, const Str256& name, const ResourcePath& resourceName, const ResourcePath& resourceLocation, ObjectType type, U32 flagMask)
    : SceneNode(parentCache,
                descriptorHash,
                name,
                resourceName,
                resourceLocation,
                SceneNodeType::TYPE_OBJECT3D,
                to_base(ComponentType::TRANSFORM) | to_base(ComponentType::BOUNDS) | to_base(ComponentType::RENDERING)),
    _context(context),
    _geometryPartitionIDs{},
    _geometryFlagMask(flagMask),
    _geometryType(type),
    _rigidBodyShape(RigidBodyShape::SHAPE_COUNT)
{
    _geometryPartitionIDs.fill(std::numeric_limits<U16>::max());
    _geometryPartitionIDs[0] = 0u;

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
            if (true) { // general meshes? Maybe have a concave flag?
                _rigidBodyShape = RigidBodyShape::SHAPE_TRIANGLEMESH;
            } else { 
                if (true) { // skinned characters?
                    _rigidBodyShape = RigidBodyShape::SHAPE_CAPSULE;
                } else { // have a convex flag for imported meshes?
                    _rigidBodyShape = RigidBodyShape::SHAPE_CONVEXMESH;
                }
            }
            } break;
        default:
            _rigidBodyShape = RigidBodyShape::SHAPE_COUNT;
            break;
    };

    _editorComponent.name(getTypeName());
}


const char* Object3D::getTypeName() const {
    if (_geometryType._value != ObjectType::COUNT) {
        return _geometryType._to_string();
    }

    return SceneNode::getTypeName();
}

bool Object3D::isPrimitive() const noexcept {
    return _geometryType._value == ObjectType::BOX_3D ||
           _geometryType._value == ObjectType::QUAD_3D ||
           _geometryType._value == ObjectType::PATCH_3D ||
           _geometryType._value == ObjectType::SPHERE_3D;
}

void Object3D::postLoad(SceneGraphNode* sgn) {
     computeTriangleList();
     SceneNode::postLoad(sgn);
}

void Object3D::rebuildVB() {
}

void Object3D::rebuild() {
    if (_geometryDirty) {
        OPTICK_EVENT();

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

VertexBuffer* Object3D::getGeometryVB() const {
    return _buffer;
}

void Object3D::onRefreshNodeData(const SceneGraphNode* sgn,
                                 const RenderStagePass& renderStagePass,
                                 const Camera& crtCamera,
                                 bool refreshData,
                                 GFX::CommandBuffer& bufferInOut) {
    if (refreshData) {
        rebuild();
    }

    SceneNode::onRefreshNodeData(sgn, renderStagePass, crtCamera, refreshData, bufferInOut);
}

void Object3D::buildDrawCommands(SceneGraphNode* sgn,
                                 const RenderStagePass& renderStagePass,
                                 const Camera& crtCamera,
                                 RenderPackage& pkgInOut) {
    VertexBuffer* vb = getGeometryVB();
    if (vb != nullptr) {
        if (pkgInOut.drawCommandCount() == 0) {
            const U16 partitionID = _geometryPartitionIDs[0];
            GenericDrawCommand cmd;
            cmd._primitiveType = PrimitiveType::TRIANGLE_STRIP;
            cmd._sourceBuffer = vb->handle();
            cmd._bufferIndex = renderStagePass.baseIndex();
            cmd._cmd.indexCount = to_U32(vb->getPartitionIndexCount(partitionID));
            cmd._cmd.firstIndex = to_U32(vb->getPartitionOffset(partitionID));
            cmd._cmd.primCount = sgn->instanceCount();
            enableOption(cmd, CmdRenderOptions::RENDER_INDIRECT);

            pkgInOut.add(GFX::DrawCommand{ cmd });
        }

        U16 prevID = 0;
        for (U8 i = 0; i < to_U8(_geometryPartitionIDs.size()); ++i) {
            U16 id = _geometryPartitionIDs[i];
            if (id == std::numeric_limits<U16>::max()) {
                assert(i > 0);
                id = prevID;
            }
            pkgInOut.setLoDIndexOffset(i, vb->getPartitionOffset(id), vb->getPartitionIndexCount(id));
            prevID = id;
        }
    }

    SceneNode::buildDrawCommands(sgn, renderStagePass, crtCamera, pkgInOut);
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
    // We can't have a VB without vertex positions
    DIVIDE_ASSERT(!geometry->getVertices().empty(),
                  "Object3D error: computeTriangleList called with no position "
                  "data available!");

    size_t partitionOffset = geometry->getPartitionOffset(_geometryPartitionIDs[0]);
    size_t partitionCount = geometry->getPartitionIndexCount(_geometryPartitionIDs[0]);
    PrimitiveType type = (_geometryType._value == ObjectType::MESH ||
                          _geometryType._value == ObjectType::SUBMESH
                              ? PrimitiveType::TRIANGLES
                              : PrimitiveType::TRIANGLE_STRIP);

    if (!_geometryTriangles.empty()) {
        _geometryTriangles.resize(0);
    }

    if (geometry->getIndexCount() == 0) {
        return false;
    }

    size_t indiceCount = partitionCount;
    if (type == PrimitiveType::TRIANGLE_STRIP) {
        size_t indiceStart = 2 + partitionOffset;
        size_t indiceEnd = indiceCount + partitionOffset;
        vec3<U32> curTriangle;
        _geometryTriangles.reserve(indiceCount / 2);
        const vectorEASTL<U32>& indices = geometry->getIndices();
        for (size_t i = indiceStart; i < indiceEnd; i++) {
            curTriangle.set(indices[i - 2], indices[i - 1], indices[i]);
            // Check for correct winding
            if (i % 2 != 0) {
                std::swap(curTriangle.y, curTriangle.z);
            }
            _geometryTriangles.push_back(curTriangle);
        }
    } else if (type == PrimitiveType::TRIANGLES) {
        indiceCount /= 3;
        _geometryTriangles.reserve(indiceCount);
        const vectorEASTL<U32>& indices = geometry->getIndices();
        for (size_t i = 0; i < indiceCount; i += 3) {
            _geometryTriangles.push_back(vec3<U32>(indices[i + 0],
                                                    indices[i + 1],
                                                    indices[i + 2]));
        }
    }

    // Check for degenerate triangles
    _geometryTriangles.erase(
        eastl::partition(
            eastl::begin(_geometryTriangles), eastl::end(_geometryTriangles),
            [](const vec3<U32>& triangle) -> bool {
                return (triangle.x != triangle.y && triangle.x != triangle.z &&
                        triangle.y != triangle.z);
            }),
        eastl::end(_geometryTriangles));

    DIVIDE_ASSERT(!_geometryTriangles.empty(),
                  "Object3D error: computeTriangleList() failed to generate "
                  "any triangles!");
    return true;
}

vectorEASTL<SceneGraphNode*> Object3D::filterByType(const vectorEASTL<SceneGraphNode*>& nodes, ObjectType filter) {
    vectorEASTL<SceneGraphNode*> result;
    result.reserve(nodes.size());

    for (SceneGraphNode* ptr : nodes) {
        if (ptr && ptr->getNode<Object3D>().getObjectType() == filter) {
            result.push_back(ptr);
        }
    };

    return result;
}

void Object3D::playAnimations(const SceneGraphNode* sgn, const bool state) {
    if (getObjectFlag(ObjectFlag::OBJECT_FLAG_SKINNED)) {
        AnimationComponent* animComp = sgn->get<AnimationComponent>();
        if (animComp != nullptr) {
            animComp->playAnimations(state);
        }
        sgn->forEachChild([state](const SceneGraphNode* child, I32 /*childIdx*/) {
            AnimationComponent* animComp = child->get<AnimationComponent>();
            // Not all submeshes are necessarily animated. (e.g. flag on the back of a character)
            if (animComp != nullptr) {
                animComp->playAnimations(state && animComp->playAnimations());
            }
            return true;
        });
    }
}

void Object3D::saveCache(ByteBuffer& outputBuffer) const {
    if (isPrimitive()) {
        outputBuffer << to_U8(getObjectType());
    } else {
        outputBuffer << stringImpl(resourceName().c_str());
    }

    SceneNode::saveCache(outputBuffer);
};

void Object3D::loadCache(ByteBuffer& inputBuffer) {
    if (isPrimitive()) {
        U8 index = 0u;
        inputBuffer >> index;
        assert(index == to_U8(getObjectType()));
    } else {
        stringImpl tempName = {};
        inputBuffer >> tempName;
        assert(tempName == resourceName().c_str());
    }
    SceneNode::loadCache(inputBuffer);
}

void Object3D::saveToXML(boost::property_tree::ptree& pt) const {
    if (static_cast<const Object3D*>(this)->isPrimitive()) {
        pt.put("model", static_cast<const Object3D*>(this)->getObjectType()._to_string());
    } else {
        pt.put("model", resourceName().c_str());
    }

    SceneNode::saveToXML(pt);
}

void Object3D::loadFromXML(const boost::property_tree::ptree& pt) {
    stringImpl temp;
    if (static_cast<const Object3D*>(this)->isPrimitive()) {
        temp = pt.get("model", getObjectType()._to_string());
        assert(temp == getObjectType()._to_string());
    } else {
        temp = pt.get("model", resourceName().c_str());
        assert(temp == resourceName().c_str());
    }

    SceneNode::loadFromXML(pt);
}

};
