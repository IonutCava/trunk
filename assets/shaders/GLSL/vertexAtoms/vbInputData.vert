invariant gl_Position;

out vec4 _vertexW;
out vec2 _texCoord;

uniform int lodLevel = 0;

uniform mat4 dvd_WorldMatrix;//[MAX_INSTANCES];
uniform mat3 dvd_NormalMatrix;
uniform mat4 dvd_WorldViewMatrix;
uniform mat4 dvd_WorldViewMatrixInverse;
uniform mat4 dvd_WorldViewProjectionMatrix;


layout(std140) uniform dvd_MatrixBlock
{
   mat4 dvd_ProjectionMatrix;
   mat4 dvd_ViewMatrix;
   mat4 dvd_ViewProjectionMatrix;
};

#include "clippingPlanes.vert"
#if defined(USE_GPU_SKINNING)
#include "boneTransforms.vert"
#endif

vec4  dvd_Vertex;
vec4  dvd_Color;
vec3  dvd_Normal;
vec3  dvd_Tangent;
vec3  dvd_BiTangent;

void computeData(){

    dvd_Vertex     = vec4(inVertexData,1.0);
    dvd_Normal     = inNormalData;
    dvd_Tangent    = inTangentData;
    dvd_BiTangent  = inBiTangentData;
    dvd_Color      = inColorData;

    #if defined(USE_GPU_SKINNING)
    applyBoneTransforms(dvd_Vertex, dvd_Normal, lodLevel);
    #endif

    _texCoord = inTexCoordData;
    _vertexW  = dvd_WorldMatrix * dvd_Vertex;

    setClipPlanes(_vertexW);
}




