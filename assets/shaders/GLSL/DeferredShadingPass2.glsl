-- Vertex
#include "vertexDefault.vert"

uniform vec3 dvd_cameraPosition;
out vec3 _eyePos;

void main(void)
{

	computeData();
	_eyePos = dvd_cameraPosition;
}

-- Fragment

uniform sampler2D albedoTexture;
uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform sampler2D blendTexture;
uniform sampler2D lightTexture;
uniform int lightCount;

in vec2 _texCoord;
in vec3 _eyePos;
out vec4 _colorOut;

void main( void )
{

   vec3 diffuse = vec3(0,0,0);
   vec3 light = vec3(0,0,0);
   vec3 lightDir = vec3(0,0,0);
   vec3 vHalfVector = vec3(0,0,0);
   vec4 albedo   = texture( albedoTexture, _texCoord );
   vec4 position = texture( positionTexture, _texCoord );
   vec4 normal   = texture( normalTexture, _texCoord );
   vec4 blend    = texture( blendTexture, _texCoord );
   vec3 eyeDir = normalize(_eyePos-position.xyz);

   float lightIntensity = 0;
   float selfLighting = position.a;
   normal.w = 0;
   normal = normalize(normal);
   // Diffusive

   for(int i=0; i<lightCount; i++){
      light = texture( lightTexture,vec2(0.01,i*0.99/lightCount) ).xyz;
      lightDir = light - position.xyz ;
      float lightDistance = length(lightDir);
      lightDir /= lightDistance;
      lightIntensity = 1 / ( 1.0 + 0.00005 * lightDistance + 0.00009 * pow(lightDistance,2));

      vec3 lightColor = texture( lightTexture, vec2(0.99,i*0.99/lightCount) ).xyz;
      vHalfVector = normalize(lightDir+eyeDir);

      float localDiffuse = max(dot(normal.xyz,lightDir),0);
      diffuse += lightColor * localDiffuse * lightIntensity;

    }

   _colorOut = vec4(diffuse,1) * albedo +   selfLighting * (albedo);
} 