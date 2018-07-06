#include "Headers/Object3D.h"
#include "Managers/Headers/SceneManager.h"

void Object3D::render(SceneGraphNode* const sgn){
	GFX_DEVICE.renderModel(sgn->getNode<Object3D>());
}

void Object3D::onDraw(){
	if(_refreshVBO){
		_geometry->Refresh();
		_refreshVBO = false;
	}
	SceneNode::onDraw();
}

void Object3D::computeTangents(){
	//Code from: http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping/#header-1
    // inputs
    std::vector<vec3<F32> > & vertices = _geometry->getPosition();
    std::vector<vec2<F32> > & uvs      = _geometry->getTexcoord();
    std::vector<vec3<F32> > & normals  = _geometry->getNormal();
    // outputs
    std::vector<vec3<F32> > & tangents = _geometry->getTangent();
    std::vector<vec3<F32> > & bitangents = _geometry->getBiTangent();

	for ( U32 i=0; i< vertices.size(); i+=3){
 		// Shortcuts for vertices
		vec3<F32> & v0 = vertices[i+0];
		vec3<F32> & v1 = vertices[i+1];
		vec3<F32> & v2 = vertices[i+2];
	 
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