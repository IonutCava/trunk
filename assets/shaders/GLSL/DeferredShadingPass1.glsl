-- Vertex
#include "vboInputData.vert"
out vec3 normals;
out vec3 position;
out mat4 TBN;

void main( void ){
	computeData();
	gl_Position = dvd_ModelViewProjectionMatrix * dvd_Vertex;

	position = vec3(transpose(dvd_ModelViewMatrix) * dvd_Vertex);
	normals = normalize(dvd_NormalMatrix * dvd_Normal);

	vec3 t = normalize(dvd_NormalMatrix * dvd_Tangent);
	vec3 n = normalize(dvd_NormalMatrix * dvd_Normal);
	vec3 b = cross(n, t);

	
   TBN = mat4( t.x, b.x, n.x, 0,
               t.y, b.y, n.y, 0,
               t.z, b.z, n.z, 0,
               0, 0, 0, 1 );

} 

-- Fragment

in vec3 normals;
in vec3 position;

out vec4 diffuseOutput; // layout(location = 0)
out vec4 posOutput;     // layout(location = 1)
out vec4 normOutput;    // layout(location = 2)
out vec4 blendOutput;   // layout(location = 3)

uniform mat4 material;
in mat4 TBN;

void main( void ){

	vec4 color = material[1]; //diffuse
    if(color.a < ALPHA_DISCARD_THRESHOLD) discard;

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

uniform sampler2D texDiffuse0;

void main( void ){
   vec4 color = texture(texDiffuse0,_texCoord);
   if(color.a < ALPHA_DISCARD_THRESHOLD) discard;

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

uniform sampler2D  texDiffuse0;
uniform sampler2D  texBump;

void main( void ){

   vec4 color = texture(texDiffuse0,_texCoord);
   if(color.a < ALPHA_DISCARD_THRESHOLD) discard;
   
   diffuseOutput = color;
   posOutput     = vec4(position,1);
   normOutput    = (texture(texBump,_texCoord) * 2 -
                     vec4(1,1,1,0)) * TBN;
   blendOutput.rgb = color.rgb * color.a; // Pre multiplied alpha
   blendOutput.a = color.a;

}

-- Vertex.Impostor

uniform mat3 dvd_NormalMatrix;
uniform mat4 dvd_ModelViewMatrix;
uniform mat4 dvd_ModelViewProjectionMatrix;

in vec3  inVertexData;
in vec3  inNormalData;

out vec3 normals;
out vec3 position;

void main( void ){
	vec4 dvd_Vertex     = vec4(inVertexData,1.0);
	vec3 dvd_Normal     = inNormalData;

	gl_Position = dvd_ModelViewProjectionMatrix * dvd_Vertex;

	position = vec3(transpose(dvd_ModelViewMatrix) * dvd_Vertex);
	normals = normalize(dvd_NormalMatrix * dvd_Normal);
} 

-- Fragment.Impostor

in vec3	 position;
in vec3	 normals;

out vec4 diffuseOutput; // layout(location = 0)
out vec4 posOutput;     // layout(location = 1)
out vec4 normOutput;    // layout(location = 2)
out vec4 blendOutput;   // layout(location = 3)

uniform mat4 material;

void main( void )
{
    vec4 color = material[1]; //diffuse
    if(color.a < ALPHA_DISCARD_THRESHOLD) discard;

	diffuseOutput	= color;
	posOutput  		= vec4(position,1);
	normOutput    	= vec4(normals,1);
    blendOutput.rgb = color.rgb * color.a; // Pre multiplied alpha
    blendOutput.a   = color.a;
}