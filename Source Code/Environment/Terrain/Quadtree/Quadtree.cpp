#include "Headers/Quadtree.h"
#include "Headers/QuadtreeNode.h"
#include "Hardware/Video/Headers/GFXDevice.h"

void Quadtree::DrawGround(bool drawInReflection) {
    assert(_root);
    _root->DrawGround(drawInReflection ? CHUNK_BIT_WATERREFLECTION : 0, _parentVBO);
}

void Quadtree::DrawGrass(U32 geometryIndex, Transform* const parentTransform)
{
    assert(_root);
    _root->DrawGrass(geometryIndex,parentTransform);
}

void Quadtree::DrawBBox() {
    assert(_root);
    _root->DrawBBox();
}

QuadtreeNode* Quadtree::FindLeaf(const vec2<F32>& pos) {
    assert(_root);
    QuadtreeNode* node = _root;

    while(!node->isALeaf()) {
        I32 i=0;
        for(i=0; i<4; i++) {
            QuadtreeNode* child = &(node->getChildren()[i]);
            if(child->getBoundingBox().ContainsPoint( vec3<F32>(pos.x, child->getBoundingBox().getCenter().y, pos.y) ))
            {
                node = child;
                break;
            }
        }

        if(i>=4) {
            return NULL;
        }
    }

    return node;
}

void Quadtree::Build(BoundingBox& terrainBBox,
                     vec2<U32>& HMsize,
                     U32 minHMSize,
                     VertexBufferObject* const groundVBO){
    _root = New QuadtreeNode();
    _root->setBoundingBox(terrainBBox);
    _root->setParentShaderProgram(_parentShaderProgram);

    _root->Build(0, vec2<U32>(0,0), HMsize, minHMSize,groundVBO,_chunkCount);

    GenerateIndexBuffer(HMsize, groundVBO);
}

void Quadtree::GenerateGrassIndexBuffer(U32 bilboardCount){
    _root->GenerateGrassIndexBuffer(bilboardCount);
}

void Quadtree::GenerateIndexBuffer(vec2<U32>& HMsize, VertexBufferObject* const groundVBO){
    const U32 terrainWidth  = HMsize.x;
    const U32 terrainHeight = HMsize.y;
    const U32 numTriangles = ( terrainWidth - 1 ) * ( terrainHeight - 1 ) * 2;
    groundVBO->reserveIndexCount(numTriangles * 3);
    groundVBO->computeTriangles(false);

    vec3<U32> firstTri, secondTri;

    for (U32 j = 0; j < (terrainHeight - 1); ++j ) {
        for (U32 i = 0; i < (terrainWidth - 1); ++i )  {
            I32 vertexIndex = ( j * terrainWidth ) + i;
            // Top triangle (T0)
            firstTri.set(vertexIndex, vertexIndex + terrainWidth + 1, vertexIndex + 1);
            groundVBO->addIndex(firstTri.x); // V0
            groundVBO->addIndex(firstTri.y); // V3
            groundVBO->addIndex(firstTri.z); // V1
            groundVBO->getTriangles().push_back(firstTri);

            // Bottom triangle (T1)
            secondTri.set(vertexIndex, vertexIndex + terrainWidth, vertexIndex + terrainWidth + 1);
            groundVBO->addIndex(secondTri.x); // V0
            groundVBO->addIndex(secondTri.y); // V2
            groundVBO->addIndex(secondTri.z); // V3
            groundVBO->getTriangles().push_back(secondTri);
        }
    }
}

BoundingBox& Quadtree::computeBoundingBox(const vectorImpl<vec3<F32> >& vertices){
    assert(_root);
    assert(!vertices.empty());
     _root->computeBoundingBox(vertices);
     return _root->getBoundingBox();;
}

void Quadtree::Destroy(){
    SAFE_DELETE(_root);
}