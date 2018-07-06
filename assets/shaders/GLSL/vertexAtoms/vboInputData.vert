struct Light{
	vec4  _position;
	vec4  _direction;
	vec4  _ambient;
	vec4  _diffuse;
	vec4  _specular;
	vec4  _attenuation; //x = constAtt, y = linearAtt, z = quadraticAtt, w = range
	float _spotExponent;
	float _spotCutoff;
	float _brightness;
	float _padding;
};

vec4  dvd_Vertex;
vec3  dvd_Normal;
vec3  dvd_Tangent;
vec3  dvd_BiTangent;
vec4  dvd_BoneWeight;
ivec4 dvd_BoneIndice;

out vec2 _texCoord;

in vec3  inVertexData;
in vec3  inNormalData;
in vec2  inTexCoordData;
in vec3  inTangentData;
in vec3  inBiTangentData;
in vec4  inBoneWeightData;
in ivec4 inBoneIndiceData;

uniform mat4 dvd_ModelMatrix;
uniform mat3 dvd_NormalMatrix;
uniform mat4 dvd_ModelViewMatrix;
uniform mat4 dvd_ModelViewMatrixInverse;
uniform mat4 dvd_ModelViewProjectionMatrix;

layout(std140) uniform dvd_MatrixBlock
{
    mat4 dvd_ProjectionMatrix;
	mat4 dvd_ViewMatrix;
};

//layout(std140) uniform dvd_LightBlock
//{
//	Light _LightSources[MAX_LIGHT_COUNT];
//};

void computeData(){

	dvd_Vertex     = vec4(inVertexData,1.0);
	dvd_Normal     = inNormalData;
	_texCoord      = inTexCoordData;
	dvd_Tangent    = inTangentData;
	dvd_BiTangent  = inBiTangentData;
}

void computeBoneData(){
    dvd_BoneWeight = inBoneWeightData;
    dvd_BoneIndice = inBoneIndiceData;
}


