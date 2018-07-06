-- Vertex

attribute vec3  inVertexData;
attribute vec2  inTexCoordData;

vec4  vertexData;

varying vec4 _vertexMV;
varying vec2 _texCoord;

uniform mat4 projectionMatrix;

void computeData(void){

#if defined(USE_VBO_DATA)
    	vertexData = vec4(inVertexData,1.0);
		_texCoord   = inTexCoordData;
#else
		vertexData = gl_Vertex;
		_texCoord   = gl_MultiTexCoord0.xy;
#endif

}

void main(void){

	computeData();
	// Transformed position 
	_vertexMV = gl_ModelViewMatrix * vertexData;
	//Compute the final vert position
	gl_Position = projectionMatrix  * _vertexMV;
}

-- Vertex.WithBones

attribute vec3  inVertexData;
attribute vec2  inTexCoordData;
attribute vec4  inBoneWeightData;
attribute ivec4 inBoneIndiceData;

vec4  vertexData;
vec4  boneWeightData;
ivec4 boneIndiceData;

varying vec4 _vertexMV;
varying vec2 _texCoord;

uniform bool hasAnimations;
uniform mat4 boneTransforms[60];
uniform mat4 projectionMatrix;

void computeData(void){

#if defined(USE_VBO_DATA)
		vertexData = vec4(inVertexData,1.0);
		_texCoord       = inTexCoordData;
		boneWeightData = inBoneWeightData;
		boneIndiceData = inBoneIndiceData;

#else
		vertexData     = gl_Vertex;
		_texCoord       = gl_MultiTexCoord0.xy;
		boneWeightData = gl_MultiTexCoord3;
		boneIndiceData = ivec4(gl_MultiTexCoord4);
#endif

}
void applyBoneTransforms(inout vec4 position){
	if(hasAnimations) {
	 // ///w - weight value
	  boneWeightData.w = 1.0 - dot(boneWeightData.xyz, vec3(1.0, 1.0, 1.0));

	   vec4 newPosition  = boneWeightData.x * boneTransforms[boneIndiceData.x] * position;
		    newPosition += boneWeightData.y * boneTransforms[boneIndiceData.y] * position;
		    newPosition += boneWeightData.z * boneTransforms[boneIndiceData.z] * position;
		    newPosition += boneWeightData.w * boneTransforms[boneIndiceData.w] * position;

	  position = newPosition;
	}
}

void main(void){

	computeData();
	applyBoneTransforms(vertexData);
	// Transformed position 
	_vertexMV = projectionMatrix  * gl_ModelViewMatrix * vertexData;
	//Compute the final vert position
	gl_Position = _vertexMV;
	gl_FrontColor = gl_Color;
}

-- Fragment

varying vec2 _texCoord;
varying vec4 _vertexMV;

//Opacity and specular maps
uniform float opacity;
uniform bool hasOpacity;
uniform sampler2D opacityMap;

void main(){
	if(hasOpacity ){
		if(texture(opacityMap, _texCoord).a < 0.2) discard;
	}else{
		if(opacity < 0.2) discard;
	}

	float depth = _vertexMV.z / _vertexMV.w ;
	depth = depth * 0.5 + 0.5;
	float moment1 = depth;
	float moment2 = depth * depth;
	
	// Adjusting moments (this is sort of bias per pixel) using partial derivative
	float dx = dFdx(depth);
	float dy = dFdy(depth);
	moment2 += 0.25*(dx*dx+dy*dy) ;
	gl_FragData[0] =  vec4( moment1,moment2, 0.0, 0.0 );
}