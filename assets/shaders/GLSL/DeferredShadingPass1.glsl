-- Vertex
#include "vbInputData.vert"
out vec3 normals;
out vec3 position;
out mat4 TBN;

void main( void ){
    computeData();
    gl_Position = dvd_ViewProjectionMatrix * _vertexW;

    position = vec3(transpose(dvd_ViewMatrix * dvd_WorldMatrix) * dvd_Vertex);
    normals = normalize(dvd_NormalMatrix * dvd_Normal);
    vec3 t = normalize(dvd_NormalMatrix * dvd_Tangent);
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
    mat 4 worldView = dvd_ViewMatrix * dvd_WorldMatrix;
    gl_Position = dvd_ProjectionMatrix * worldView * dvd_Vertex;

    position = vec3(transpose(worldView) * dvd_Vertex);
    normals = normalize(dvd_NormalMatrix * dvd_Normal);
} 

-- Fragment

in vec3 normals;
in vec3 position;

out vec4 diffuseOutput; // layout(location = 0)
out vec4 posOutput;     // layout(location = 1)
out vec4 normOutput;    // layout(location = 2)
out vec4 blendOutput;   // layout(location = 3)

in mat4 TBN;

void main( void ){

    vec4 color = dvd_MatDiffuse; //diffuse

    diffuseOutput = color;
    posOutput     = vec4(position,1);
    normOutput    = vec4(normals,1);
    blendOutput.rgb = color.rgb * color.a; // Pre multiplied alpha
    blendOutput.a = color.a;
}

-- Fragment.Texture

in vec3  normals;
in vec3  position;
in vec2  _texCoord;
in mat4  TBN;

out vec4 diffuseOutput; // layout(location = 0)
out vec4 posOutput;     // layout(location = 1)
out vec4 normOutput;    // layout(location = 2)
out vec4 blendOutput;   // layout(location = 3)

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;

void main( void ){
   vec4 color = texture(texDiffuse0,_texCoord);

   diffuseOutput = color;
   posOutput     = vec4(position,1);
   normOutput    = vec4(normals,1);
   blendOutput.rgb = color.rgb * color.a; // Pre multiplied alpha
   blendOutput.a = color.a;
}

-- Fragment.Bump

in vec3       position;
in vec2       _texCoord;
in mat4       TBN;

out vec4 diffuseOutput; // layout(location = 0)
out vec4 posOutput;     // layout(location = 1)
out vec4 normOutput;    // layout(location = 2)
out vec4 blendOutput;   // layout(location = 3)

layout(binding = TEXTURE_UNIT0)     uniform sampler2D  texDiffuse0;
layout(binding = TEXTURE_NORMALMAP) uniform sampler2D  texNormalMap;

void main( void ){

   vec4 color = texture(texDiffuse0,_texCoord);
   
   diffuseOutput = color;
   posOutput     = vec4(position,1);
   normOutput    = (texture(texNormalMap,_texCoord) * 2 -
                     vec4(1,1,1,0)) * TBN;
   blendOutput.rgb = color.rgb * color.a; // Pre multiplied alpha
   blendOutput.a = color.a;

}

-- Fragment.Impostor

in vec3	 position;
in vec3	 normals;

out vec4 diffuseOutput; // layout(location = 0)
out vec4 posOutput;     // layout(location = 1)
out vec4 normOutput;    // layout(location = 2)
out vec4 blendOutput;   // layout(location = 3)

void main( void )
{
    vec4 color = dvd_MatDiffuse; //diffuse

    diffuseOutput	= color;
    posOutput  		= vec4(position,1);
    normOutput    	= vec4(normals,1);
    blendOutput.rgb = color.rgb * color.a; // Pre multiplied alpha
    blendOutput.a   = color.a;
}