-- Vertex

in vec3  inVertexData;
in vec2  inTexCoordData;
vec4  dvd_Vertex;

#if defined(USE_GPU_SKINNING)
in vec4  inBoneWeightData;
in ivec4 inBoneIndiceData;

vec4  dvd_BoneWeight;
ivec4 dvd_BoneIndice;

uniform bool hasAnimations;
uniform mat4 boneTransforms[60];
#endif

out vec4 _vertexMV;
out vec2 _texCoord;
uniform mat4 dvd_ModelViewMatrix;

layout(std140) uniform dvd_MatrixBlock
{
    mat4 dvd_ProjectionMatrix;
	mat4 dvd_ViewMatrix;
};

void computeData(void){

	dvd_Vertex = vec4(inVertexData,1.0);
	_texCoord       = inTexCoordData;

#if defined(USE_GPU_SKINNING)
	dvd_BoneWeight = inBoneWeightData;
	dvd_BoneIndice = inBoneIndiceData;
#endif
}

#if defined(USE_GPU_SKINNING)
void applyBoneTransforms(inout vec4 position){
	if(hasAnimations) {
	 // ///w - weight value
	  dvd_BoneWeight.w = 1.0 - dot(dvd_BoneWeight.xyz, vec3(1.0, 1.0, 1.0));

	   vec4 newPosition  = dvd_BoneWeight.x * boneTransforms[dvd_BoneIndice.x] * position;
		    newPosition += dvd_BoneWeight.y * boneTransforms[dvd_BoneIndice.y] * position;
		    newPosition += dvd_BoneWeight.z * boneTransforms[dvd_BoneIndice.z] * position;
		    newPosition += dvd_BoneWeight.w * boneTransforms[dvd_BoneIndice.w] * position;

	  position = newPosition;
	}
}
#endif

void main(void){

	computeData();
#if defined(USE_GPU_SKINNING)
	applyBoneTransforms(dvd_Vertex);
#endif
	// Transformed position 
	_vertexMV = dvd_ModelViewMatrix * dvd_Vertex;
	//Compute the final vert position
	gl_Position = dvd_ProjectionMatrix * _vertexMV;
}

-- Fragment

in vec2 _texCoord;
in vec4 _vertexMV;
out vec4 _colorOut;
//Opacity and specular maps
uniform float opacity;
uniform sampler2D opacityMap;

void main(){

#if defined(USE_OPACITY_MAP)
	if(texture(opacityMap, _texCoord).a < ALPHA_DISCARD_THRESHOLD) discard;
#else
	if(opacity < ALPHA_DISCARD_THRESHOLD) discard;
#endif

	float depth = _vertexMV.z / _vertexMV.w ;
	depth = depth * 0.5 + 0.5;
	float moment1 = depth;
	float moment2 = depth * depth;
	
	// Adjusting moments (this is sort of bias per pixel) using partial derivative
	float dx = dFdx(depth);
	float dy = dFdy(depth);
	moment2 += 0.25*(dx*dx+dy*dy) ;
	_colorOut =  vec4( moment1,moment2, 0.0, 0.0 );
}