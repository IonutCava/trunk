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

out vec4 _vertexM;
out vec2 _texCoord;
out float outClip;

#if defined(SKIP_HARDWARE_CLIPPING)
out float dvd_ClipDistance[MAX_CLIP_PLANES];
#else
out float gl_ClipDistance[MAX_CLIP_PLANES];
#endif

uniform int dvd_clip_plane_count;

in vec3  inVertexData;
in vec3  inNormalData;
in vec2  inTexCoordData;
in vec3  inTangentData;
in vec3  inBiTangentData;
in vec4  inBoneWeightData;
in ivec4 inBoneIndiceData;

uniform mat4 dvd_ModelMatrix;//[MAX_INSTANCES];
uniform mat3 dvd_NormalMatrix;
uniform mat4 dvd_ModelViewMatrix;
uniform mat4 dvd_ModelViewMatrixInverse;
uniform mat4 dvd_ModelViewProjectionMatrix;
uniform vec4 dvd_clip_plane[MAX_CLIP_PLANES];

layout(std140) uniform dvd_MatrixBlock
{
    mat4 dvd_ProjectionMatrix;
    mat4 dvd_ViewMatrix;
};

//layout(std140) uniform dvd_LightBlock
//{
//	Light _LightSources[MAX_LIGHT_COUNT];
//};

void setClipPlanes(){
#if defined(SKIP_HARDWARE_CLIPPING)

#if MAX_CLIP_PLANES > 0
    dvd_ClipDistance[0] = dot(_vertexM, dvd_clip_plane[0]);
#	if MAX_CLIP_PLANES > 1
        if(dvd_clip_plane_count == 1) return;
        dvd_ClipDistance[1] = dot(_vertexM, dvd_clip_plane[1]) ;
#		if MAX_CLIP_PLANES > 2
            if(dvd_clip_plane_count == 2) return;
            dvd_ClipDistance[2] = dot(_vertexM, dvd_clip_plane[2]);
#			if MAX_CLIP_PLANES > 3
                if(dvd_clip_plane_count == 3) return;
                dvd_ClipDistance[3] = dot(_vertexM, dvd_clip_plane[3]);
#				if MAX_CLIP_PLANES > 4
                    if(dvd_clip_plane_count == 4) return;
                    dvd_ClipDistance[4] = dot(_vertexM, dvd_clip_plane[4]);
#					if MAX_CLIP_PLANES > 5	
                        if(dvd_clip_plane_count == 5) return;
                        dvd_ClipDistance[5] = dot(_vertexM, dvd_clip_plane[5]);
#					endif //5
#				endif //4
#			endif //3
#		endif //2
#	endif //1
#endif //0

#else //skip clip planes

#if MAX_CLIP_PLANES > 0
    gl_ClipDistance[0] = dot(_vertexM, dvd_clip_plane[0]);
#	if MAX_CLIP_PLANES > 1
        if(dvd_clip_plane_count == 1) return;
        gl_ClipDistance[1] = dot(_vertexM, dvd_clip_plane[1]) ;
#		if MAX_CLIP_PLANES > 2
            if(dvd_clip_plane_count == 2) return;
            gl_ClipDistance[2] = dot(_vertexM, dvd_clip_plane[2]);
#			if MAX_CLIP_PLANES > 3
                if(dvd_clip_plane_count == 3) return;
                gl_ClipDistance[3] = dot(_vertexM, dvd_clip_plane[3]);
#				if MAX_CLIP_PLANES > 4
                    if(dvd_clip_plane_count == 4) return;
                    gl_ClipDistance[4] = dot(_vertexM, dvd_clip_plane[4]);
#					if MAX_CLIP_PLANES > 5	
                        if(dvd_clip_plane_count == 5) return;
                        gl_ClipDistance[5] = dot(_vertexM, dvd_clip_plane[5]);
#					endif //5
#				endif //4
#			endif //3
#		endif //2
#	endif //1
#endif //0

#endif //skip clip planes
}

void computeData(){

    dvd_Vertex     = vec4(inVertexData,1.0);
    dvd_Normal     = inNormalData;
    _texCoord      = inTexCoordData;
    dvd_Tangent    = inTangentData;
    dvd_BiTangent  = inBiTangentData;
    _vertexM       = dvd_ModelMatrix * dvd_Vertex;
    setClipPlanes();
}

void computeBoneData(){
    dvd_BoneWeight = inBoneWeightData;
    dvd_BoneIndice = inBoneIndiceData;
}


