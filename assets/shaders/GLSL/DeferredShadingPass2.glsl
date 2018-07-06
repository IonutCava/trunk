-- Vertex
#include "vboInputData.vert"
attribute vec3 cameraPosition;
varying vec3 _eyePos;

void main( void ){
   computeData();
   _eyePos = cameraPosition;
   gl_Position = gl_ModelViewProjectionMatrix * vertexData;
   gl_FrontColor = vec4(1.0, 1.0, 1.0, 1.0);
} 

-- Fragment

uniform sampler2D tImage0;
uniform sampler2D tImage1;
uniform sampler2D tImage2;
uniform sampler2D lightTexture;
uniform int lightCount;

varying vec2 _texCoord;
varying vec3 _eyePos;

void main( void )
{

   vec3 diffuse = vec3(0,0,0);
   vec3 light = vec3(0,0,0);
   vec3 lightDir = vec3(0,0,0);
   vec3 vHalfVector = vec3(0,0,0);
   vec4 image0 = texture( tImage0, _texCoord );
   vec4 position = texture( tImage1, _texCoord );
   vec4 normal = texture( tImage2, _texCoord );
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

   gl_FragColor = vec4(diffuse,1) * image0 +   selfLighting * (image0);
} 