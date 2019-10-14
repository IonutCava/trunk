#include "stdafx.h"

#include "Headers/VertexBuffer.h"

#include "Platform/Video/Headers/RenderAPIWrapper.h"

namespace Divide {

vectorEASTL<AttribFlags> VertexBuffer::_attribMasks;
VertexBuffer::VertexBuffer(GFXDevice& context)
    : VertexDataInterface(context),
     _format(GFXDataFormat::UNSIGNED_SHORT),
     _primitiveRestartEnabled(false),
     _staticBuffer(false),
     _keepDataInMemory(false)
{
    assert(handle()._id != 0);
    reset();
}

VertexBuffer::~VertexBuffer()
{
    reset();
}

bool VertexBuffer::create(bool staticDraw) {
    _staticBuffer = staticDraw;
    return true;
}

bool VertexBuffer::createInternal() {
    return true;
}


void VertexBuffer::setAttribMasks(size_t count, const AttribFlags& flagMask) {
    _attribMasks.resize(0);
    _attribMasks.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        _attribMasks.push_back(flagMask);
    }
}

void VertexBuffer::setAttribMask(size_t index, const AttribFlags& flagMask) {
    assert(index < _attribMasks.size());
    _attribMasks[index] = flagMask;
}

void VertexBuffer::computeNormals() {
    vec3<F32> U, V, normal;
    // Code from
    // http://devmaster.net/forums/topic/1065-calculating-normals-of-a-mesh/

    size_t vertCount = getVertexCount();
    size_t indexCount = getIndexCount();

    typedef vector<vec3<F32>> normalVector;

    vector<normalVector> normalBuffer(vertCount);
    for (U32 i = 0; i < indexCount; i += 3) {

        U32 idx0 = getIndex(i + 0);
        if (idx0 == Config::PRIMITIVE_RESTART_INDEX_L ||
            idx0 == Config::PRIMITIVE_RESTART_INDEX_S) {
            assert(i > 2);
            i -= 2;
            continue;
        }

        U32 idx1 = getIndex(i + 1);
        U32 idx2 = getIndex(i + 2);
        // get the three vertices that make the faces
        const vec3<F32>& p1 = getPosition(idx0);
        const vec3<F32>& p2 = getPosition(idx1);
        const vec3<F32>& p3 = getPosition(idx2);

        U.set(p2 - p1);
        V.set(p3 - p1);
        normal.cross(U, V);
        normal.normalize();

        // Store the face's normal for each of the vertices that make up the face.
        normalBuffer[idx0].push_back(normal);
        normalBuffer[idx1].push_back(normal);
        normalBuffer[idx2].push_back(normal);
    }

    // Now loop through each vertex vector, and average out all the normals stored.
    vec3<F32> currentNormal;
    for (U32 i = 0; i < vertCount; ++i) {
        currentNormal.reset();
        for (U32 j = 0; j < normalBuffer[i].size(); ++j) {
            currentNormal += normalBuffer[i][j];
        }
        currentNormal /= to_F32(normalBuffer[i].size());

        modifyNormalValue(i, currentNormal);
    }

    normalBuffer.clear();
}

void VertexBuffer::computeTangents() {
    size_t indexCount = getIndexCount();

    // Code from:
    // http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping/#header-1

    vec3<F32> deltaPos1, deltaPos2;
    vec2<F32> deltaUV1, deltaUV2;
    vec3<F32> tangent;

    for (U32 i = 0; i < indexCount; i += 3) {
        // get the three vertices that make the faces
        U32 idx0 = getIndex(i + 0);
        if (idx0 == Config::PRIMITIVE_RESTART_INDEX_L ||
            idx0 == Config::PRIMITIVE_RESTART_INDEX_S) {
            assert(i > 2);
            i -= 2;
            continue;
        }

        U32 idx1 = getIndex(i + 1);
        U32 idx2 = getIndex(i + 2);

        const vec3<F32>& v0 = getPosition(idx0);
        const vec3<F32>& v1 = getPosition(idx1);
        const vec3<F32>& v2 = getPosition(idx2);

        // Shortcuts for UVs
        const vec2<F32>& uv0 = getTexCoord(idx0);
        const vec2<F32>& uv1 = getTexCoord(idx1);
        const vec2<F32>& uv2 = getTexCoord(idx2);

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
        modifyTangentValue(idx0, tangent);
        modifyTangentValue(idx1, tangent);
        modifyTangentValue(idx2, tangent);
    }
}

void VertexBuffer::fromBuffer(VertexBuffer& other) {
    reset();
    _staticBuffer = other._staticBuffer;
    _format = other._format;
    _partitions = other._partitions;
    _primitiveRestartEnabled = other._primitiveRestartEnabled;
    _attribDirty = other._attribDirty;
    unchecked_copy(_indices, other._indices);
    unchecked_copy(_data, other._data);
}

bool VertexBuffer::deserialize(ByteBuffer& dataIn) {
    assert(!dataIn.empty());
    U64 idString;
    dataIn >> idString;
    if (idString == _ID("VB")) {
        reset();
        U32 format;

        dataIn >> _staticBuffer;
        dataIn >> _keepDataInMemory;
        dataIn >> format;
        _format = static_cast<GFXDataFormat>(format);
        dataIn >> _partitions;
        dataIn >> _indices;
        dataIn >> _data;
        dataIn >> _attribDirty;
        dataIn >> _primitiveRestartEnabled;

        return true;
    }

    return false;
}

bool VertexBuffer::serialize(ByteBuffer& dataOut) const {
    if (!_data.empty()) {
        dataOut << _ID("VB");
        dataOut << _staticBuffer;
        dataOut << _keepDataInMemory;
        dataOut << to_U32(_format);
        dataOut << _partitions;
        dataOut << _indices;
        dataOut << _data;
        dataOut << _attribDirty;
        dataOut << _primitiveRestartEnabled;

        return true;
    }
    return false;
}

};