-- Vertex
#include "vbInputData.vert"
out vec3 normals;
out vec3 position;
out mat4 TBN;

void main( void ){
    computeData();
    gl_Position = dvd_ViewProjectionMatrix * VAR._vertexW;

    position = vec3(transpose(dvd_ViewMatrix * dvd_WorldMatrix(VAR.dvd_instanceID)) * dvd_Vertex);
    normals = normalize(dvd_NormalMatrixWV(VAR.dvd_instanceID) * dvd_Normal);
    vec3 t = normalize(dvd_NormalMatrixWV(VAR.dvd_instanceID) * dvd_Tangent);
    vec3 n = normals;
    vec3 b = cross(n, t);

    
   TBN = mat4( t.x, b.x, n.x, 0,
               t.y, b.y, n.y, 0,
               t.z, b.z, n.z, 0,
               0, 0, 0, 1 );

} 

-- Vertex.Impostor

out vec3 normals;
out vec3 position;

void main( void ){
    vec4 dvd_Vertex     = vec4(inVertexData,1.0);
    vec3 dvd_Normal     = inNormalData;
    mat 4 worldView = dvd_ViewMatrix * dvd_WorldMatrix(VAR.dvd_instanceID);
    gl_Position = dvd_ProjectionMatrix * worldView * dvd_Vertex;

    position = vec3(transpose(worldView) * dvd_Vertex);
    normals = normalize(dvd_NormalMatrixWV(VAR.dvd_instanceID) * dvd_Normal);
} 

-- Fragment

#include "materialData.frag"

in vec3       position;
in mat4       TBN;

out vec4 diffuseOutput; // layout(location = 0)
out vec4 posOutput;     // layout(location = 1)
out vec4 normOutput;    // layout(location = 2)
out vec4 blendOutput;   // layout(location = 3)

layout(binding = TEXTURE_UNIT0)     uniform sampler2D  texDiffuse0;
layout(binding = TEXTURE_NORMALMAP) uniform sampler2D  texNormalMap;

void main() {
   vec4 colour = getAlbedo();
   
   diffuseOutput = colour;
   posOutput     = vec4(position,1);

#if defined(COMPUTE_TBN)
   normOutput = (texture(texNormalMap, VAR._texCoord) * 2 - vec4(1,1,1,0)) * TBN;
#else
   normOutput = vec4(normals, 1);
#endif
#if defined (USE_DOUBLE_SIDED)
   normOutput = gl_FrontFacing ? normOutput : -normOutput;
#endif

   blendOutput.rgb = colour.rgb * colour.a; // Pre multiplied alpha
   blendOutput.a = colour.a;

}