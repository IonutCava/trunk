attribute vec3  inVertexData;
attribute vec3  inNormalData;
attribute vec2  inTexCoordData;
attribute vec3  inTangentData;
attribute vec3  inBiTangentData;
attribute vec4  inBoneWeightData;
attribute ivec4 inBoneIndiceData;

vec4  vertexData;
vec3  normalData;
vec3  tangentData;
vec3  biTangentData;
vec4  boneWeightData;
ivec4 boneIndiceData;

varying vec2 _texCoord;

uniform mat4 transformMatrix;
uniform mat4 parentTransformMatrix;
uniform mat4 projectionMatrix;
uniform mat4 modelViewInvMatrix;
uniform mat4 modelViewProjectionMatrix;


void computeData(void){

#if defined(USE_VBO_DATA)

		vertexData     = gl_Vertex;
		normalData     = inNormalData;
		_texCoord      = inTexCoordData;
		tangentData    = inTangentData;
		biTangentData  = inBiTangentData;
		boneWeightData = inBoneWeightData;
		boneIndiceData = inBoneIndiceData;
		

#else
		vertexData     = gl_Vertex;
		normalData     = gl_Normal;
		_texCoord      = gl_MultiTexCoord0.xy;
		tangentData    = gl_MultiTexCoord1.xyz;
		biTangentData  = gl_MultiTexCoord2.xyz;
		boneWeightData = gl_MultiTexCoord3;
		boneIndiceData = ivec4(gl_MultiTexCoord4);
#endif

}


