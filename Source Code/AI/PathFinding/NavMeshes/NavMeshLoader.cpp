#include "Headers/NavMeshLoader.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Water/Headers/Water.h"

#include <fstream>

namespace Divide {
namespace AI {
namespace Navigation {
namespace NavigationMeshLoader {
static vec3<F32> _minVertValue, _maxVertValue;
static const U32 cubeFaces[6][4] = {{0, 4, 6, 2},
                                    {0, 2, 3, 1},
                                    {0, 1, 5, 4},
                                    {3, 2, 6, 7},
                                    {7, 6, 4, 5},
                                    {3, 7, 5, 1}};

char* parseRow(char* buf, char* bufEnd, char* row, I32 len) {
    bool cont = false;
    bool start = true;
    bool done = false;
    I32 n = 0;

    while (!done && buf < bufEnd) {
        char c = *buf;
        buf++;
        // multi row
        switch (c) {
            case '\\':
                cont = true;
                break;  // multi row
            case '\n': {
                if (start)
                    break;
                done = true;
            } break;
            case '\r':
                break;
            case '\t':
            case ' ':
                if (start)
                    break;
            default: {
                start = false;
                cont = false;
                row[n++] = c;
                if (n >= len - 1)
                    done = true;
            } break;
        }
    }
    ACKNOWLEDGE_UNUSED(cont);
    row[n] = '\0';
    return buf;
}

I32 parseFace(char* row, I32* data, I32 n, I32 vcnt) {
    I32 j = 0;
    while (*row != '\0') {
        // Skip initial white space
        while (*row != '\0' && (*row == ' ' || *row == '\t'))
            row++;
        char* s = row;
        // Find vertex delimiter and terminated the string there for conversion.
        while (*row != '\0' && *row != ' ' && *row != '\t') {
            if (*row == '/')
                *row = '\0';
            row++;
        }

        if (*s == '\0')
            continue;

        I32 vi = atoi(s);
        data[j++] = vi < 0 ? vi + vcnt : vi - 1;
        if (j >= n)
            return j;
    }
    return j;
}

bool loadMeshFile(NavModelData& outData, const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp)
        return false;

    outData.setName(filename);
    fseek(fp, 0, SEEK_END);

    I32 bufSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buf = MemoryManager_NEW char[bufSize];
    if (!buf) {
        fclose(fp);
        return false;
    }

    fread(buf, bufSize, 1, fp);
    fclose(fp);

    char* src = buf;
    char* srcEnd = buf + bufSize;
    char row[512];
    I32 face[32];
    F32 x, y, z;
    I32 nv;
    I32 result;
    while (src < srcEnd) {
        // Parse one row
        row[0] = '\0';
        src = parseRow(src, srcEnd, row, sizeof(row) / sizeof(char));
        // Skip comments
        if (row[0] == '#')
            continue;

        if (row[0] == 'v' && row[1] != 'n' && row[1] != 't') {
            // Vertex pos
            result = sscanf(row + 1, "%f %f %f", &x, &y, &z);
            if (result != 0)
                addVertex(&outData, vec3<F32>(x, y, z));
        }
        if (row[0] == 'f') {
            // Faces
            nv = parseFace(row + 1, face, 32, outData._vertexCount);
            for (I32 i = 2; i < nv; ++i) {
                const I32 a = face[0];
                const I32 b = face[i - 1];
                const I32 c = face[i];
                if (a < 0 || a >= (I32)outData._vertexCount || b < 0 ||
                    b >= (I32)outData._vertexCount || c < 0 ||
                    c >= (I32)outData._vertexCount) {
                    continue;
                }

                addTriangle(&outData, vec3<U32>(a, b, c));
            }
        }
    }

    MemoryManager::DELETE_ARRAY(buf);

    // Calculate normals.
    outData._normals = MemoryManager_NEW F32[outData._triangleCount * 3];

    for (I32 i = 0; i < (I32)outData._triangleCount * 3; i += 3) {
        const F32* v0 = &outData._vertices[outData._triangles[i] * 3];
        const F32* v1 = &outData._vertices[outData._triangles[i + 1] * 3];
        const F32* v2 = &outData._vertices[outData._triangles[i + 2] * 3];

        F32 e0[3], e1[3];
        for (I32 j = 0; j < 3; ++j) {
            e0[j] = v1[j] - v0[j];
            e1[j] = v2[j] - v0[j];
        }

        F32* n = &outData._normals[i];
        n[0] = e0[1] * e1[2] - e0[2] * e1[1];
        n[1] = e0[2] * e1[0] - e0[0] * e1[2];
        n[2] = e0[0] * e1[1] - e0[1] * e1[0];
        F32 d = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        if (d > 0) {
            d = 1.0f / d;
            n[0] *= d;
            n[1] *= d;
            n[2] *= d;
        }
    }

    return true;
}

bool saveMeshFile(const NavModelData& inData, const char* filename) {
    if (!inData.getVertCount() || !inData.getTriCount())
        return false;

    // Create the file if it doesn't exists
    std::ofstream myfile;
    myfile.open(filename, std::fstream::out);
    if (!myfile.is_open()) {
        return false;
    }

    F32* vstart = inData._vertices;
    I32* tstart = inData._triangles;
    for (U32 i = 0; i < inData.getVertCount(); i++) {
        F32* vp = vstart + i * 3;
        myfile << "v " << (*(vp)) << " " << *(vp + 1) << " " << (*(vp + 2))
               << "\n";
    }
    for (U32 i = 0; i < inData.getTriCount(); i++) {
        I32* tp = tstart + i * 3;
        myfile << "f " << (*(tp) + 1) << " " << (*(tp + 1) + 1) << " "
               << (*(tp + 2) + 1) << "\n";
    }

    myfile.close();
    return true;
}

NavModelData mergeModels(NavModelData& a,
                         NavModelData& b,
                         bool delOriginals /* = false*/) {
    NavModelData mergedData;
    if (a.getVerts() || b.getVerts()) {
        if (!a.getVerts())
            return b;
        else if (!b.getVerts())
            return a;

        mergedData.clear();

        I32 totalVertCt = (a.getVertCount() + b.getVertCount());
        I32 newCap = 8;

        while (newCap < totalVertCt)
            newCap *= 2;

        mergedData._vertices = MemoryManager_NEW F32[newCap * 3];
        mergedData._vertexCapacity = newCap;
        mergedData._vertexCount = totalVertCt;

        memcpy(mergedData._vertices, a.getVerts(),
               a.getVertCount() * 3 * sizeof(F32));
        memcpy(mergedData._vertices + a.getVertCount() * 3, b.getVerts(),
               b.getVertCount() * 3 * sizeof(F32));

        I32 totalTriCt = (a.getTriCount() + b.getTriCount());
        newCap = 8;

        while (newCap < totalTriCt)
            newCap *= 2;

        mergedData._triangles = MemoryManager_NEW I32[newCap * 3];
        mergedData._triangleCapacity = newCap;
        mergedData._triangleCount = totalTriCt;
        I32 aFaceSize = a.getTriCount() * 3;
        memcpy(mergedData._triangles, a.getTris(), aFaceSize * sizeof(I32));

        I32 bFaceSize = b.getTriCount() * 3;
        I32* bFacePt = mergedData._triangles +
                       a.getTriCount() * 3;  // i like pointing at faces
        memcpy(bFacePt, b.getTris(), bFaceSize * sizeof(I32));

        for (U32 i = 0; i < to_uint(bFaceSize); i++) {
            *(bFacePt + i) += a.getVertCount();
        }

        if (mergedData._vertexCount > 0) {
            if (delOriginals) {
                a.clear();
                b.clear();
            }
        } else
            mergedData.clear();
    }

    mergedData.setName(a.getName() + "+" + b.getName());
    return mergedData;
}

void addVertex(NavModelData* modelData, const vec3<F32>& vertex) {
    if (modelData->getVertCount() + 1 > modelData->_vertexCapacity) {
        modelData->_vertexCapacity =
            !modelData->_vertexCapacity ? 8 : modelData->_vertexCapacity * 2;

        F32* nv = MemoryManager_NEW F32[modelData->_vertexCapacity * 3];

        if (modelData->getVertCount()) {
            memcpy(nv, modelData->getVerts(),
                   modelData->getVertCount() * 3 * sizeof(F32));
        }
        if (modelData->getVerts()) {
            MemoryManager::DELETE_ARRAY(modelData->_vertices);
        }
        modelData->_vertices = nv;
    }

    F32* dst = &modelData->_vertices[modelData->getVertCount() * 3];
    *dst++ = vertex.x;
    *dst++ = vertex.y;
    *dst++ = vertex.z;

    modelData->_vertexCount++;
}

void addTriangle(NavModelData* modelData,
                 const vec3<U32>& triangleIndices,
                 U32 triangleIndexOffset,
                 const SamplePolyAreas& areaType) {
    if (modelData->getTriCount() + 1 > modelData->_triangleCapacity) {
        modelData->_triangleCapacity = !modelData->_triangleCapacity
                                           ? 8
                                           : modelData->_triangleCapacity * 2;
        I32* nv = MemoryManager_NEW I32[modelData->_triangleCapacity * 3];

        if (modelData->getTriCount()) {
            memcpy(nv, modelData->_triangles,
                   modelData->getTriCount() * 3 * sizeof(I32));
        }
        if (modelData->getTris()) {
            MemoryManager::DELETE_ARRAY(modelData->_triangles);
        }
        modelData->_triangles = nv;
    }

    I32* dst = &modelData->_triangles[modelData->getTriCount() * 3];

    *dst++ = to_int(triangleIndices.x + triangleIndexOffset);
    *dst++ = to_int(triangleIndices.y + triangleIndexOffset);
    *dst++ = to_int(triangleIndices.z + triangleIndexOffset);

    modelData->getAreaTypes().push_back(areaType);
    modelData->_triangleCount++;
}

const vec3<F32> borderOffset(BORDER_PADDING);
bool parse(const BoundingBox& box, NavModelData& outData, std::weak_ptr<SceneGraphNode> sgn) {
    SceneGraphNode_ptr nodeSGN(sgn.lock());
    assert(nodeSGN != nullptr);

    if (nodeSGN->getComponent<NavigationComponent>()->navigationContext() !=
            NavigationComponent::NavigationContext::NODE_IGNORE &&  // Ignore if
                                                                    // specified
        box.getHeight() > 0.05f)  // Skip small objects
    {
        SceneNode* sn = nodeSGN->getNode();

        SceneNodeType nodeType = sn->getType();
        U32 ignoredNodeType = to_uint(SceneNodeType::TYPE_ROOT) |
                              to_uint(SceneNodeType::TYPE_LIGHT) |
                              to_uint(SceneNodeType::TYPE_PARTICLE_EMITTER) |
                              to_uint(SceneNodeType::TYPE_TRIGGER) |
                              to_uint(SceneNodeType::TYPE_SKY) |
                              to_uint(SceneNodeType::TYPE_VEGETATION_GRASS);

        U32 allowedNodeType = to_uint(SceneNodeType::TYPE_WATER) |
                              to_uint(SceneNodeType::TYPE_OBJECT3D) |
                              to_uint(SceneNodeType::TYPE_VEGETATION_TREES);

        if (!BitCompare(allowedNodeType, to_uint(nodeType))) {
            if (!BitCompare(ignoredNodeType, to_uint(nodeType))) {
                Console::printfn(Locale::get("WARN_NAV_UNSUPPORTED"),
                                 sn->getName().c_str());
                goto next;
            }
        }

        if (nodeType == SceneNodeType::TYPE_OBJECT3D) {
            Object3D::ObjectType crtType =
                dynamic_cast<Object3D*>(sn)->getObjectType();
            if (crtType == Object3D::ObjectType::TEXT_3D ||
                crtType == Object3D::ObjectType::MESH ||
                crtType == Object3D::ObjectType::FLYWEIGHT) {
                goto next;
            }
        }

        MeshDetailLevel level = MeshDetailLevel::MAXIMUM;
        VertexBuffer* geometry = nullptr;
        SamplePolyAreas areaType = SamplePolyAreas::SAMPLE_AREA_OBSTACLE;

        switch (nodeType) {
            case SceneNodeType::TYPE_WATER: {
                if (!nodeSGN->getComponent<NavigationComponent>()
                         ->navMeshDetailOverride())
                    level = MeshDetailLevel::BOUNDINGBOX;
                areaType = SamplePolyAreas::SAMPLE_POLYAREA_WATER;
            } break;
            case SceneNodeType::TYPE_OBJECT3D: {
                // Check if we need to override detail level
                if (!nodeSGN->getComponent<NavigationComponent>()
                         ->navMeshDetailOverride() &&
                    nodeSGN->usageContext() ==
                        SceneGraphNode::UsageContext::NODE_STATIC) {
                    level = MeshDetailLevel::BOUNDINGBOX;
                }
                if (dynamic_cast<Object3D*>(sn)->getObjectType() ==
                    Object3D::ObjectType::TERRAIN) {
                    areaType = SamplePolyAreas::SAMPLE_POLYAREA_GROUND;
                }
            } break;
            default: {
                assert(false);  // we should never reach this due to the bit
                                // checks above
            } break;
        };

        // I should remove this hack - Ionut
        if (nodeType == SceneNodeType::TYPE_WATER) {
            nodeSGN = nodeSGN->getChildren()["waterPlane"];
        }

        Console::d_printfn(Locale::get("NAV_MESH_CURRENT_NODE"),
                           sn->getName().c_str(), to_uint(level));

        U32 currentTriangleIndexOffset = outData.getVertCount();

        if (level == MeshDetailLevel::MAXIMUM) {
            if (nodeType == SceneNodeType::TYPE_OBJECT3D) {
                geometry = dynamic_cast<Object3D*>(sn)->getGeometryVB();
            } else /*nodeType == TYPE_WATER*/ {
                geometry =
                    dynamic_cast<WaterPlane*>(sn)->getQuad()->getGeometryVB();
            }
            assert(geometry != nullptr);

            const vectorImpl<VertexBuffer::Vertex >& vertices = geometry->getVertices();
            if (vertices.empty()) {
                return false;
            }

            dynamic_cast<Object3D*>(sn)->computeTriangleList();
            const vectorImpl<vec3<U32> >& triangles =
                dynamic_cast<Object3D*>(sn)->getTriangles();
            if (nodeType != SceneNodeType::TYPE_OBJECT3D ||
                (nodeType == SceneNodeType::TYPE_OBJECT3D &&
                 dynamic_cast<Object3D*>(sn)->getObjectType() !=
                     Object3D::ObjectType::TERRAIN)) {
                mat4<F32> nodeTransform =
                    nodeSGN->getComponent<PhysicsComponent>()->getWorldMatrix();
                for (U32 i = 0; i < vertices.size(); ++i) {
                    // Apply the node's transform and add the vertex to the
                    // NavMesh
                    addVertex(&outData, nodeTransform * (vertices[i]._position));
                }
            } else {
                for (U32 i = 0; i < vertices.size(); ++i) {
                    // Apply the node's transform and add the vertex to the
                    // NavMesh
                    addVertex(&outData, (vertices[i]._position));
                }
            }

            for (U32 i = 0; i < triangles.size(); ++i) {
                addTriangle(&outData, triangles[i], currentTriangleIndexOffset,
                            areaType);
            }
        } else if (level == MeshDetailLevel::BOUNDINGBOX) {
            const vec3<F32>* vertices = box.getPoints();

            for (U32 i = 0; i < 8; i++) {
                addVertex(&outData, (vertices[i]));
            }

            for (U32 f = 0; f < 6; f++) {
                for (U32 v = 2; v < 4; v++) {
                    // Note: We reverse the normal on the polygons to prevent
                    // things from going inside out
                    addTriangle(&outData,
                                vec3<U32>(cubeFaces[f][0], cubeFaces[f][v - 1],
                                          cubeFaces[f][v]),
                                currentTriangleIndexOffset, areaType);
                }
            }
        } else {
            Console::errorfn(Locale::get("ERROR_RECAST_LEVEL"), level);
        }

        Console::printfn(Locale::get("NAV_MESH_ADD_NODE"),
                         sn->getName().c_str());
    }
// although labels are bad, skipping here using them is the easiest solution to
// follow -Ionut
next:
    ;
    for (SceneGraphNode::NodeChildren::value_type& it : nodeSGN->getChildren()) {
        if (!parse(it.second->getBoundingBoxConst(), outData, it.second)) {
            return false;
        }
    }

    return true;
}
};
};  // namespace Navigation
};  // namespace AI
};  // namespace Divide
