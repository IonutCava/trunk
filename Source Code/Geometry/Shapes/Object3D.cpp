#include "Headers/Object3D.h"

#include "Managers/Headers/SceneManager.h"
#include "Hardware/Video/Headers/GFXDevice.h"

	Object3D::Object3D(ObjectType type, PrimitiveType vboType, ObjectFlag flag) : 
											  SceneNode(TYPE_OBJECT3D),
											  _update(false),
											  _geometryType(type),
											  _geometryFlag(flag),
											  _geometry(GFX_DEVICE.newVBO(vboType)),
											  _refreshVBO(true)

	{}

	Object3D::Object3D(const std::string& name, ObjectType type, PrimitiveType vboType, ObjectFlag flag) : 
																		SceneNode(name,TYPE_OBJECT3D),
																	    _update(false),
																		_geometryType(type),
																		_geometryFlag(flag),
																		_geometry(GFX_DEVICE.newVBO(vboType)),
																		_refreshVBO(true)
	{}

void Object3D::render(SceneGraphNode* const sgn){
	GFX_DEVICE.renderModel(sgn->getNode<Object3D>());
}

VertexBufferObject* const Object3D::getGeometryVBO() {
	assert(_geometry != NULL);
	if(_refreshVBO){
		_geometry->queueRefresh();
		_refreshVBO = false;
	}
	
	return _geometry;
}

void Object3D::onDraw(){

	SceneNode::onDraw();

	if(getMaterial()){
		ShaderProgram* finalStageShader = getMaterial()->getShaderProgram(FINAL_STAGE);
		if(finalStageShader && getMaterial()->shaderProgramChanged()){
			_geometry->setShaderProgram(finalStageShader);
		}
	}
	
}

void Object3D::computeTangents(){
	//Code from: http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping/#header-1
    // inputs
    vectorImpl<vec2<F32> > & uvs      = _geometry->getTexcoord();
    //vectorImpl<vec3<F32> > & normals  = _geometry->getNormal();
    // outputs
    vectorImpl<vec3<F32> > & tangents = _geometry->getTangent();
    vectorImpl<vec3<F32> > & bitangents = _geometry->getBiTangent();

	for ( U32 i=0; i< _geometry->getPosition().size(); i+=3){
 		// Shortcuts for vertices
		vec3<F32>  v0 = _geometry->getPosition(i+0);
		vec3<F32>  v1 = _geometry->getPosition(i+1);
		vec3<F32>  v2 = _geometry->getPosition(i+2);
	 
		// Shortcuts for UVs
		vec2<F32> & uv0 = uvs[i+0];
		vec2<F32> & uv1 = uvs[i+1];
		vec2<F32> & uv2 = uvs[i+2];
	 
		// Edges of the triangle : postion delta
		vec3<F32> deltaPos1 = v1-v0;
		vec3<F32> deltaPos2 = v2-v0;
	 
		// UV delta
		vec2<F32> deltaUV1 = uv1-uv0;
		vec2<F32> deltaUV2 = uv2-uv0;

		F32 r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
		vec3<F32> tangent = (deltaPos1 * deltaUV2.y   - deltaPos2 * deltaUV1.y)*r;
		vec3<F32> bitangent = (deltaPos2 * deltaUV1.x   - deltaPos1 * deltaUV2.x)*r;

		// Set the same tangent for all three vertices of the triangle.
		// They will be merged later, in vboindexer.cpp
		tangents.push_back(tangent);
		tangents.push_back(tangent);
		tangents.push_back(tangent);
 
		// Same thing for binormals
		bitangents.push_back(bitangent);
		bitangents.push_back(bitangent);
		bitangents.push_back(bitangent);
	}
}