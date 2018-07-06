#include "Headers/Terrain.h"
#include "Headers/TerrainChunk.h"
#include "Headers/TerrainDescriptor.h"

#include "Quadtree/Headers/Quadtree.h"
#include "Quadtree/Headers/QuadtreeNode.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Managers/Headers/SceneManager.h"

#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Hardware/Video/Buffers/VertexBuffer/Headers/VertexBuffer.h"

namespace Divide {

Terrain::Terrain() : Object3D(TERRAIN),
    _alphaTexturePresent(false),
    _terrainWidth(0),
    _terrainHeight(0),
    _terrainQuadtree(New Quadtree()),
    _plane(nullptr),
    _drawBBoxes(false),
    _vegetationGrassNode(nullptr),
    _underwaterDiffuseScale(100.0f),
    _terrainRenderStateHash(0),
    _terrainDepthRenderStateHash(0),
    _terrainReflectionRenderStateHash(0),
    _terrainInView(false),
    _planeInView(false),
    _planeSGN(nullptr)
{
    getGeometryVB()->useLargeIndices(true);//<32bit indices
}

Terrain::~Terrain()
{
}

bool Terrain::unload(){
    SAFE_DELETE(_terrainQuadtree);

    for (TerrainTextureLayer*& terrainTextures : _terrainTextures){
        SAFE_DELETE(terrainTextures);
    }
    _terrainTextures.clear();

    RemoveResource(_vegDetails.grassBillboards);
    return SceneNode::unload();
}

void Terrain::postLoad(SceneGraphNode* const sgn){
    assert(getState() == RES_LOADED);

    getGeometryVB()->Create();

    reserveTriangleCount((_terrainWidth - 1) * (_terrainHeight - 1) * 2);
    _terrainQuadtree->Build(_boundingBox, vec2<U32>(_terrainWidth, _terrainHeight), _chunkSize, sgn);

    ShaderProgram* drawShader = getDrawShader();
    drawShader->Uniform("dvd_waterHeight", GET_ACTIVE_SCENE()->state().getWaterLevel());
    drawShader->Uniform("bbox_min", _boundingBox.getMin());
    drawShader->Uniform("bbox_extent", _boundingBox.getExtent());
    drawShader->UniformTexture("texWaterCaustics",    ShaderProgram::TEXTURE_UNIT0);
    drawShader->UniformTexture("texUnderwaterAlbedo", ShaderProgram::TEXTURE_UNIT1);
    drawShader->UniformTexture("texUnderwaterDetail", ShaderProgram::TEXTURE_NORMALMAP);
    drawShader->Uniform("underwaterDiffuseScale", _underwaterDiffuseScale);

    U8 textureOffset = ShaderProgram::TEXTURE_NORMALMAP + 1;
    U8 layerOffset = 0;
    std::string layerIndex;
    for (U32 i = 0; i < _terrainTextures.size(); ++i){
        layerOffset = i * 2 + textureOffset;
        layerIndex = Util::toString(i);
        TerrainTextureLayer* textureLayer = _terrainTextures[i];
        drawShader->UniformTexture("texBlend[" + layerIndex + "]",    layerOffset);
        drawShader->UniformTexture("texTileMaps[" + layerIndex + "]", layerOffset + 1);
        
        getMaterial()->addCustomTexture(textureLayer->blendMap(), layerOffset);
        getMaterial()->addCustomTexture(textureLayer->tileMaps(), layerOffset + 1);

        drawShader->Uniform("diffuseScale[" + layerIndex + "]", textureLayer->getDiffuseScales());
        drawShader->Uniform("detailScale["  + layerIndex + "]", textureLayer->getDetailScales());

    }

    ResourceDescriptor infinitePlane("infinitePlane");
    infinitePlane.setFlag(true); //No default material
    _plane = CreateResource<Quad3D>(infinitePlane);
    // our bottom plane is placed at the bottom of our water entity
    F32 height = GET_ACTIVE_SCENE()->state().getWaterLevel() - GET_ACTIVE_SCENE()->state().getWaterDepth();
    _farPlane = 2.0f * GET_ACTIVE_SCENE()->state().getRenderState().getCameraConst().getZPlanes().y;
    _plane->setCorner(Quad3D::TOP_LEFT,     vec3<F32>(-_farPlane * 1.5f, height, -_farPlane * 1.5f));
    _plane->setCorner(Quad3D::TOP_RIGHT,    vec3<F32>( _farPlane * 1.5f, height, -_farPlane * 1.5f));
    _plane->setCorner(Quad3D::BOTTOM_LEFT,  vec3<F32>(-_farPlane * 1.5f, height,  _farPlane * 1.5f));
    _plane->setCorner(Quad3D::BOTTOM_RIGHT, vec3<F32>( _farPlane * 1.5f, height,  _farPlane * 1.5f));
    _planeSGN = sgn->addNode(_plane);
    _planeSGN->setActive(false);
    _plane->computeBoundingBox(_planeSGN);
    computeBoundingBox(sgn);

    SceneNode::postLoad(sgn);
}

bool Terrain::computeBoundingBox(SceneGraphNode* const sgn){
    //The terrain's final bounding box is the QuadTree's root bounding box
    _boundingBox = _terrainQuadtree->computeBoundingBox();
    //Inform the scenegraph of our new BB
    sgn->getBoundingBox() = _boundingBox;
    return  SceneNode::computeBoundingBox(sgn);
}

bool Terrain::isInView(const SceneRenderState& sceneRenderState, const BoundingBox& boundingBox, const BoundingSphere& sphere, const bool distanceCheck){
    _terrainInView = SceneNode::isInView(sceneRenderState, boundingBox, sphere, distanceCheck);
    _planeInView = _terrainInView ? false : _plane->isInView(sceneRenderState, _planeSGN->getBoundingBoxConst(), _planeSGN->getBoundingSphereConst(), distanceCheck);

    return _terrainInView || _planeInView;
}

void Terrain::sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState){
    _terrainQuadtree->sceneUpdate(deltaTime, sgn, sceneState);
    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}

size_t Terrain::getDrawStateHash(RenderStage renderStage){
    if(GFX_DEVICE.isCurrentRenderStage(DEPTH_STAGE))
        return GFX_DEVICE.isCurrentRenderStage(Z_PRE_PASS_STAGE) ? _terrainRenderStateHash : _terrainDepthRenderStateHash;

    return GFX_DEVICE.isCurrentRenderStage(REFLECTION_STAGE) ? _terrainReflectionRenderStateHash : _terrainRenderStateHash;
}

ShaderProgram* const Terrain::getDrawShader(RenderStage renderStage){
    if(GFX_DEVICE.isCurrentRenderStage(REFLECTION_STAGE))
        renderStage = FINAL_STAGE;

    return SceneNode::getDrawShader(renderStage);
}

void Terrain::render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState, const RenderStage& currentRenderStage){
    size_t drawStateHash = getDrawStateHash(currentRenderStage);
    ShaderProgram* drawShader = getDrawShader(currentRenderStage);
    I32 drawID = GFX_DEVICE.getDrawID(sgn->getGUID());

    if(_terrainInView){
        // draw ground
        _terrainQuadtree->CreateDrawCommands(sceneRenderState);


        std::sort(_drawCommands.begin(), _drawCommands.end(),
                  [](const GenericDrawCommand& a, const GenericDrawCommand& b) {
                        return a.LoD() < b.LoD();
                    });

        for(GenericDrawCommand& cmd : _drawCommands){
            cmd.renderWireframe(sgn->renderWireframe());
            cmd.stateHash(drawStateHash);
            cmd.shaderProgram(drawShader);
            cmd.drawID(drawID);
        }

        GFX_DEVICE.submitRenderCommand(getGeometryVB(), drawCommands());

        clearDrawCommands();
    }
    
    // draw infinite plane
    if (GFX_DEVICE.isCurrentRenderStage(FINAL_STAGE | Z_PRE_PASS_STAGE) && _planeInView ){
        GenericDrawCommand cmd(TRIANGLE_STRIP, 0, 0);
        cmd.renderWireframe(sgn->renderWireframe());
        cmd.stateHash(drawStateHash);
        cmd.drawID(drawID);
        cmd.shaderProgram(drawShader);
        GFX_DEVICE.submitRenderCommand(_plane->getGeometryVB(), cmd);
    }
}

void Terrain::drawBoundingBox(SceneGraphNode* const sgn) const {
    if(_drawBBoxes)	_terrainQuadtree->DrawBBox();
}

vec3<F32> Terrain::getPositionFromGlobal(F32 x, F32 z) const {
    x -= _boundingBox.getCenter().x;
    z -= _boundingBox.getCenter().z;
    F32 xClamp = (0.5f * _terrainWidth) + x;
    F32 zClamp = (0.5f * _terrainHeight) - z;
    xClamp /= _terrainWidth;
    zClamp /= _terrainHeight;
    zClamp = 1 - zClamp;
    vec3<F32> temp = getPosition(xClamp,zClamp);

    return temp;
}

vec3<F32> Terrain::getPosition(F32 x_clampf, F32 z_clampf) const{
    assert(!(x_clampf<.0f || z_clampf<.0f || x_clampf>1.0f || z_clampf>1.0f));

    vec2<F32>  posF(x_clampf * _terrainWidth, z_clampf * _terrainHeight );
    vec2<I32>  posI((I32)(posF.x),   (I32)(posF.y) );
    vec2<F32>  posD(posF.x - posI.x, posF.y - posI.y );

    if(posI.x >= (I32)_terrainWidth - 1)	posI.x = _terrainWidth  - 2;
    if(posI.y >= (I32)_terrainHeight - 1)	posI.y = _terrainHeight - 2;

    assert(posI.x >= 0 && posI.x < (I32)(_terrainWidth)-1 && posI.y >= 0 && posI.y < (I32)(_terrainHeight) - 1);

    vec3<F32> pos(_boundingBox.getMin().x + x_clampf * (_boundingBox.getMax().x - _boundingBox.getMin().x), 0.0f,
                  _boundingBox.getMin().z + z_clampf * (_boundingBox.getMax().z - _boundingBox.getMin().z));

    const vectorImpl<vec3<F32> >& position = getGeometryVB()->getPosition();

    pos.y = (position[TER_COORD(posI.x,     posI.y,     (I32)_terrainWidth)].y) * (1.0f - posD.x) * (1.0f - posD.y)
          + (position[TER_COORD(posI.x + 1, posI.y,     (I32)_terrainWidth)].y) *         posD.x  * (1.0f - posD.y)
          + (position[TER_COORD(posI.x,     posI.y + 1, (I32)_terrainWidth)].y) * (1.0f - posD.x) *         posD.y
          + (position[TER_COORD(posI.x + 1, posI.y + 1, (I32)_terrainWidth)].y) *         posD.x  *         posD.y;

    return pos;
}

vec3<F32> Terrain::getNormal(F32 x_clampf, F32 z_clampf) const{
    assert(!(x_clampf<.0f || z_clampf<.0f || x_clampf>1.0f || z_clampf>1.0f));

    vec2<F32>  posF(	x_clampf * _terrainWidth, z_clampf * _terrainHeight );
    vec2<I32>  posI(	(I32)(x_clampf * _terrainWidth), (I32)(z_clampf * _terrainHeight) );
    vec2<F32>  posD(	posF.x - posI.x, posF.y - posI.y );

    if(posI.x >= (I32)(_terrainWidth)-1)    posI.x = (I32)(_terrainWidth) - 2;
    if(posI.y >= (I32)(_terrainHeight)-1)	posI.y = (I32)(_terrainHeight) - 2;

    assert(posI.x >= 0 && posI.x < (I32)(_terrainWidth)-1 && posI.y >= 0 && posI.y < (I32)(_terrainHeight)-1);

    const vectorImpl<vec3<F32> >& normals = getGeometryVB()->getNormal();

    return (normals[TER_COORD(posI.x,     posI.y,     (I32)_terrainWidth)]) * (1.0f - posD.x) * (1.0f - posD.y)
         + (normals[TER_COORD(posI.x + 1, posI.y,     (I32)_terrainWidth)]) *         posD.x  * (1.0f - posD.y)
         + (normals[TER_COORD(posI.x,     posI.y + 1, (I32)_terrainWidth)]) * (1.0f - posD.x) *         posD.y
         + (normals[TER_COORD(posI.x + 1, posI.y + 1, (I32)_terrainWidth)]) *         posD.x  *         posD.y;
}

vec3<F32> Terrain::getTangent(F32 x_clampf, F32 z_clampf) const{
    assert(!(x_clampf<.0f || z_clampf<.0f || x_clampf>1.0f || z_clampf>1.0f));

    vec2<F32> posF(	x_clampf * _terrainWidth, z_clampf * _terrainHeight );
    vec2<I32> posI(	(I32)(x_clampf * _terrainWidth), (I32)(z_clampf * _terrainHeight) );
    vec2<F32> posD(	posF.x - posI.x, posF.y - posI.y );

    if (posI.x >= (I32)(_terrainWidth)-1)    posI.x = (I32)(_terrainWidth)-2;
    if (posI.y >= (I32)(_terrainHeight)-1)	posI.y = (I32)(_terrainHeight)-2;

    assert(posI.x >= 0 && posI.x < (I32)(_terrainWidth)-1 && posI.y >= 0 && posI.y < (I32)(_terrainHeight)-1);

    const vectorImpl<vec3<F32> >& tangents = getGeometryVB()->getTangent();

    return (tangents[TER_COORD(posI.x,     posI.y,     (I32)_terrainWidth)]) * (1.0f - posD.x) * (1.0f - posD.y)
         + (tangents[TER_COORD(posI.x + 1, posI.y,     (I32)_terrainWidth)]) *         posD.x  * (1.0f - posD.y)
         + (tangents[TER_COORD(posI.x,     posI.y + 1, (I32)_terrainWidth)]) * (1.0f - posD.x) *         posD.y
         + (tangents[TER_COORD(posI.x + 1, posI.y + 1, (I32)_terrainWidth)]) *         posD.x  *         posD.y;
}

TerrainTextureLayer::~TerrainTextureLayer()
{
    RemoveResource(_blendMap);
    RemoveResource(_tileMaps);
}

};