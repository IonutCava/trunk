#include "Headers/Object3D.h"

#include "Managers/Headers/SceneManager.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"

Object3D::Object3D(const ObjectType& type, const PrimitiveType& vbType, const ObjectFlag& flag) : Object3D("", type, vbType, flag)

{
}

Object3D::Object3D(const std::string& name, const ObjectType& type, const PrimitiveType& vbType, const ObjectFlag& flag) : SceneNode(name,TYPE_OBJECT3D),
                                                                                                                           _update(false),
                                                                                                                           _geometryType(type),
                                                                                                                           _geometryFlag(flag),
                                                                                                                           _geometry(GFX_DEVICE.newVB(vbType))
{
    _geometry = vbType != PrimitiveType_PLACEHOLDER ? GFX_DEVICE.newVB(vbType) : nullptr;
    _renderInstance = New RenderInstance(this);
}

Object3D::~Object3D()
{
    SAFE_DELETE(_geometry);
    SAFE_DELETE(_renderInstance);
}

void Object3D::render(SceneGraphNode* const sgn, const SceneRenderState& sceneRenderState){
    if(!GFX_DEVICE.excludeFromStateChange(SceneNode::getType())){
        _renderInstance->transform(sgn->getTransform());
    }
    _renderInstance->stateHash(_drawStateHash);

    GFX_DEVICE.renderInstance(_renderInstance);
}

void  Object3D::postLoad(SceneGraphNode* const sgn){
    Material* mat = getMaterial();
    if (mat && getFlag() == OBJECT_FLAG_SKINNED)
        mat->setHardwareSkinning(true);

    SceneNode::postLoad(sgn);
}

bool Object3D::onDraw(SceneGraphNode* const sgn, const RenderStage& currentStage){
    if (getState() != RES_LOADED) 
        return false;

    //check if we need to update vb shader
    //custom shaders ALWAYS override material shaders
    if (_customShader){
        _drawShader = _customShader;
    } else {
        if (!getMaterial())
            return false;
        RenderStage shaderFlag =  bitCompare(DEPTH_STAGE, currentStage) ? ((bitCompare(currentStage, SHADOW_STAGE) ? SHADOW_STAGE : Z_PRE_PASS_STAGE)) : FINAL_STAGE;
        _drawShader = getMaterial()->getShaderInfo(shaderFlag).getProgram();
    }

    assert(_drawShader != nullptr);
    if(_geometry) _geometry->setShaderProgram(_drawShader);

    return true;
}

void Object3D::computeNormals() {
    if (!_geometry) return;

    vec3<F32> v1 , v2, normal;
    //Code from http://devmaster.net/forums/topic/1065-calculating-normals-of-a-mesh/

    vectorImpl<vec3<F32> >* normal_buffer = new vectorImpl<vec3<F32> >[_geometry->getPosition().size()];

    for( U32 i = 0; i < _geometry->getIndexCount(); i += 3 ) {
        // get the three vertices that make the faces
        const vec3<F32>& p1 = _geometry->getPosition(_geometry->getIndex(i+0));
        const vec3<F32>& p2 = _geometry->getPosition(_geometry->getIndex(i+1));
        const vec3<F32>& p3 = _geometry->getPosition(_geometry->getIndex(i+2));

        v1.set(p2 - p1);
        v2.set(p3 - p1);
        normal.cross(v1, v2);
        normal.normalize();

        // Store the face's normal for each of the vertices that make up the face.
        normal_buffer[_geometry->getIndex(i+0)].push_back( normal );
        normal_buffer[_geometry->getIndex(i+1)].push_back( normal );
        normal_buffer[_geometry->getIndex(i+2)].push_back( normal );
    }

    _geometry->resizeNormalCount((U32)_geometry->getPosition().size());
    // Now loop through each vertex vector, and avarage out all the normals stored.
    vec3<F32> currentNormal;
    for(U32 i = 0; i < _geometry->getPosition().size(); ++i ){
        currentNormal.reset();
        for(U32 j = 0; j < normal_buffer[i].size(); ++j ){
            currentNormal += normal_buffer[i][j];
        }
        currentNormal /= normal_buffer[i].size();

        _geometry->modifyNormalValue(i, currentNormal);
    }
    SAFE_DELETE_ARRAY(normal_buffer);
}

void Object3D::computeTangents(){
    if (!_geometry) return;

    //Code from: http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping/#header-1
    // inputs
    const vectorImpl<vec2<F32> > & uvs      = _geometry->getTexcoord();
    //const vectorImpl<vec3<F32> > & normals  = _geometry->getNormal();

    vec3<F32> deltaPos1, deltaPos2;
    vec2<F32> deltaUV1, deltaUV2;
    vec3<F32> tangent, bitangent;

    for( U32 i = 0; i < _geometry->getIndexCount(); i += 3 ) {
        // get the three vertices that make the faces
        const vec3<F32>& v0 = _geometry->getPosition(_geometry->getIndex(i+0));
        const vec3<F32>& v1 = _geometry->getPosition(_geometry->getIndex(i+1));
        const vec3<F32>& v2 = _geometry->getPosition(_geometry->getIndex(i+2));

        // Shortcuts for UVs
        const vec2<F32> & uv0 = uvs[_geometry->getIndex(i+0)];
        const vec2<F32> & uv1 = uvs[_geometry->getIndex(i+1)];
        const vec2<F32> & uv2 = uvs[_geometry->getIndex(i+2)];

        // Edges of the triangle : postion delta
        deltaPos1.set(v1-v0);
        deltaPos2.set(v2-v0);

        // UV delta
        deltaUV1.set(uv1-uv0);
        deltaUV2.set(uv2-uv0);

        F32 r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
        tangent.set((deltaPos1 * deltaUV2.y   - deltaPos2 * deltaUV1.y)*r);
        bitangent.set((deltaPos2 * deltaUV1.x   - deltaPos1 * deltaUV2.x)*r);

        // Set the same tangent for all three vertices of the triangle.
        // They will be merged later, in vbindexer.cpp
        _geometry->addTangent(tangent);
        _geometry->addTangent(tangent);
        _geometry->addTangent(tangent);
        // Same thing for binormals
        _geometry->addBiTangent(bitangent);
        _geometry->addBiTangent(bitangent);
        _geometry->addBiTangent(bitangent);
    }
}