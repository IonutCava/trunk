#include "Headers/Object3D.h"

#include "Managers/Headers/SceneManager.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Geometry/Material/Headers/Material.h"

Object3D::Object3D(const ObjectType& type,const PrimitiveType& vboType,const ObjectFlag& flag) :
                                          SceneNode(TYPE_OBJECT3D),
                                          _update(false),
                                          _geometryType(type),
                                          _geometryFlag(flag),
                                          _geometry(GFX_DEVICE.newVBO(vboType))

{
    _renderInstance = New RenderInstance(this);
}

Object3D::Object3D(const std::string& name,const ObjectType& type,const PrimitiveType& vboType,const ObjectFlag& flag) :
                                                                    SceneNode(name,TYPE_OBJECT3D),
                                                                    _update(false),
                                                                    _geometryType(type),
                                                                    _geometryFlag(flag),
                                                                    _geometry(GFX_DEVICE.newVBO(vboType))
{
    _renderInstance = New RenderInstance(this);
}

Object3D::~Object3D()
{
    SAFE_DELETE(_geometry);
    SAFE_DELETE(_renderInstance);
}

void Object3D::render(SceneGraphNode* const sgn){
    if(!GFX_DEVICE.excludeFromStateChange(SceneNode::getType())){
        _renderInstance->transform(sgn->getTransform());
    }
    GFX_DEVICE.renderInstance(_renderInstance);
}

void  Object3D::postLoad(SceneGraphNode* const sgn){
    Material* mat = getMaterial();
    if (mat && getFlag() == OBJECT_FLAG_SKINNED)
        mat->setHardwareSkinning(true);

    SceneNode::postLoad(sgn);
}

void Object3D::onDraw(const RenderStage& currentStage){
    if (getState() != RES_LOADED) return;

    SceneNode::onDraw(currentStage);

    //check if we need to update vbo shader
    if(getMaterial()){
        ShaderProgram* stageShader = getMaterial()->getShaderProgram(bitCompare(SHADOW_STAGE,currentStage) ? SHADOW_STAGE : (bitCompare(FINAL_STAGE,currentStage) ? FINAL_STAGE : Z_PRE_PASS_STAGE));
        assert(stageShader != nullptr);
        _geometry->setShaderProgram(stageShader);
    }

    //custom shaders ALWAYS override material shaders
    if(_customShader){
       _geometry->setShaderProgram(_customShader);
    }
}

void Object3D::computeNormals() {
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
        // They will be merged later, in vboindexer.cpp
        _geometry->addTangent(tangent);
        _geometry->addTangent(tangent);
        _geometry->addTangent(tangent);
        // Same thing for binormals
        _geometry->addBiTangent(bitangent);
        _geometry->addBiTangent(bitangent);
        _geometry->addBiTangent(bitangent);
    }
}