#include "Headers/Object3D.h"

#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

Object3D::Object3D(ObjectType type, ObjectFlag flag)
    : Object3D("", type, to_uint(flag))
{
}

Object3D::Object3D(ObjectType type, U32 flagMask)
    : Object3D("", type, flagMask)
{
}

Object3D::Object3D(const stringImpl& name, ObjectType type, ObjectFlag flag)
    : Object3D(name, type, to_uint(flag))
{
}

Object3D::Object3D(const stringImpl& name, ObjectType type, U32 flagMask)
    : SceneNode(name, SceneNodeType::TYPE_OBJECT3D),
      _update(false),
      _playAnimations(true),
      _geometryType(type),
      _geometryFlagMask(flagMask),
      _geometryPartitionID(0U)
{
    _buffer =
        BitCompare(_geometryFlagMask, to_uint(ObjectFlag::OBJECT_FLAG_NO_VB))
            ? nullptr
            : GFX_DEVICE.newVB();
}

Object3D::~Object3D()
{
    if (!BitCompare(_geometryFlagMask,
                    to_uint(ObjectFlag::OBJECT_FLAG_NO_VB))) {
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

bool Object3D::onDraw(SceneGraphNode& sgn, RenderStage currentStage) {
    return onDraw(currentStage);
}

bool Object3D::onDraw(RenderStage currentStage) {
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

        RenderingComponent* const renderable = sgn.getComponent<RenderingComponent>();
        VertexBuffer* const vb = getGeometryVB();

        cmd.renderGeometry(renderable->renderGeometry());
        cmd.renderWireframe(renderable->renderWireframe());
        cmd.stateHash(renderable->getDrawStateHash(renderStage));
        cmd.shaderProgram(renderable->getDrawShader(renderStage));
        cmd.sourceBuffer(vb);
        cmd.cmd().indexCount = to_uint(vb->getIndexCount());
    }

    return SceneNode::getDrawCommands(sgn, renderStage, sceneRenderState, drawCommandsOut);
}

void Object3D::computeNormals() {
    if (!getGeometryVB())
        return;

    vec3<F32> v1, v2, normal;
    // Code from
    // http://devmaster.net/forums/topic/1065-calculating-normals-of-a-mesh/

    vectorImpl<vec3<F32> >* normal_buffer = nullptr;
    normal_buffer = MemoryManager_NEW
        vectorImpl<vec3<F32> >[getGeometryVB()->getPosition().size()];

    for (U32 i = 0; i < getGeometryVB()->getIndexCount(); i += 3) {
        // get the three vertices that make the faces
        const vec3<F32>& p1 =
            getGeometryVB()->getPosition(getGeometryVB()->getIndex(i + 0));
        const vec3<F32>& p2 =
            getGeometryVB()->getPosition(getGeometryVB()->getIndex(i + 1));
        const vec3<F32>& p3 =
            getGeometryVB()->getPosition(getGeometryVB()->getIndex(i + 2));

        v1.set(p2 - p1);
        v2.set(p3 - p1);
        normal.cross(v1, v2);
        normal.normalize();

        // Store the face's normal for each of the vertices that make up the
        // face.
        normal_buffer[getGeometryVB()->getIndex(i + 0)].push_back(normal);
        normal_buffer[getGeometryVB()->getIndex(i + 1)].push_back(normal);
        normal_buffer[getGeometryVB()->getIndex(i + 2)].push_back(normal);
    }

    getGeometryVB()->resizeNormalCount(
        to_uint(getGeometryVB()->getPosition().size()));
    // Now loop through each vertex vector, and average out all the normals
    // stored.
    vec3<F32> currentNormal;
    for (U32 i = 0; i < getGeometryVB()->getPosition().size(); ++i) {
        currentNormal.reset();
        for (U32 j = 0; j < normal_buffer[i].size(); ++j) {
            currentNormal += normal_buffer[i][j];
        }
        currentNormal /= to_float(normal_buffer[i].size());

        getGeometryVB()->modifyNormalValue(i, currentNormal);
    }
    MemoryManager::DELETE_ARRAY(normal_buffer);
}

void Object3D::computeTangents() {
    if (!getGeometryVB())
        return;

    // Code from:
    // http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping/#header-1
    // inputs
    const vectorImpl<vec2<F32> >& uvs = getGeometryVB()->getTexcoord();
    // const vectorImpl<vec3<F32> > & normals  = _geometry->getNormal();

    vec3<F32> deltaPos1, deltaPos2;
    vec2<F32> deltaUV1, deltaUV2;
    vec3<F32> tangent;

    for (U32 i = 0; i < getGeometryVB()->getIndexCount(); i += 3) {
        // get the three vertices that make the faces
        const vec3<F32>& v0 =
            getGeometryVB()->getPosition(getGeometryVB()->getIndex(i + 0));
        const vec3<F32>& v1 =
            getGeometryVB()->getPosition(getGeometryVB()->getIndex(i + 1));
        const vec3<F32>& v2 =
            getGeometryVB()->getPosition(getGeometryVB()->getIndex(i + 2));

        // Shortcuts for UVs
        const vec2<F32>& uv0 = uvs[getGeometryVB()->getIndex(i + 0)];
        const vec2<F32>& uv1 = uvs[getGeometryVB()->getIndex(i + 1)];
        const vec2<F32>& uv2 = uvs[getGeometryVB()->getIndex(i + 2)];

        // Edges of the triangle : position delta
        deltaPos1.set(v1 - v0);
        deltaPos2.set(v2 - v0);

        // UV delta
        deltaUV1.set(uv1 - uv0);
        deltaUV2.set(uv2 - uv0);

        F32 r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
        tangent.set((deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r);

        // Set the same tangent for all three vertices of the triangle.
        // They will be merged later, in vbindexer.cpp
        getGeometryVB()->addTangent(tangent);
        getGeometryVB()->addTangent(tangent);
        getGeometryVB()->addTangent(tangent);
    }
}

// Create a list of triangles from the vertices + indices lists based on
// primitive type
bool Object3D::computeTriangleList(bool force) {
    VertexBuffer* geometry = getGeometryVB();
    DIVIDE_ASSERT(geometry != nullptr,
                  "Object3D error: Please specify a valid VertexBuffer before "
                  "calculating the triangle list!");

    U32 partitionOffset = geometry->getPartitionOffset(_geometryPartitionID);
    U32 partitionCount = geometry->getPartitionCount(_geometryPartitionID);
    PrimitiveType type = (_geometryType == ObjectType::MESH ||
                          _geometryType == ObjectType::SUBMESH
                              ? PrimitiveType::TRIANGLES
                              : PrimitiveType::TRIANGLE_STRIP);
    // We can't have a VB without vertex positions
    DIVIDE_ASSERT(!geometry->getPosition().empty(),
                  "Object3D error: computeTriangleList called with no position "
                  "data available!");

    if (!_geometryTriangles.empty() && force)
        _geometryTriangles.resize(0);

    if (geometry->getIndexCount() == 0)
        return false;

    U32 indiceCount = partitionCount;
    bool largeIndices = geometry->usesLargeIndices();
    if (type == PrimitiveType::TRIANGLE_STRIP) {
        U32 indiceStart = 2 + partitionOffset;
        U32 indiceEnd = indiceCount + partitionOffset;
        vec3<U32> curTriangle;
        _geometryTriangles.reserve(indiceCount / 2);
        if (largeIndices) {
            const vectorImpl<U32>& indices = geometry->getIndices<U32>();
            for (U32 i = indiceStart; i < indiceEnd; i++) {
                curTriangle.set(indices[i - 2], indices[i - 1], indices[i]);
                // Check for correct winding
                if (i % 2 != 0)
                    std::swap(curTriangle.y, curTriangle.z);
                _geometryTriangles.push_back(curTriangle);
            }
        } else {
            const vectorImpl<U16>& indices = geometry->getIndices<U16>();
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
            const vectorImpl<U32>& indices = geometry->getIndices<U32>();
            for (U32 i = 0; i < indiceCount; i++) {
                _geometryTriangles.push_back(vec3<U32>(indices[i * 3 + 0],
                                                       indices[i * 3 + 1],
                                                       indices[i * 3 + 2]));
            }
        } else {
            const vectorImpl<U16>& indices = geometry->getIndices<U16>();
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