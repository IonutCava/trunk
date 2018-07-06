-- Vertex
#include "vboInputData.vert"

void main(void){
    computeData():
	gl_Position = dvd_ModelViewProjectionMatrix * dvd_Vertex;
	gl_Position = sign( gl_Position );
 
	// Texture coordinate for screen aligned (in correct range):
	_texCoord = (vec2( gl_Position.x, - gl_Position.y ) + vec2( 1.0 ) ) * 0.5;
}

-- Fragment

uniform sampler2D rnm;
uniform sampler2D normalMap;

in  vec2 _texCoord;
out vec4 _colorOut;

uniform float totStrength;
uniform float strength;
uniform float offset;
uniform float falloff;
uniform float rad;

#define SAMPLES 10 // 10 is good
const float invSamples = -1.38/10.0;


void main(void){

	vec4 color = vec4(0.0,0.0,0.0,1.0);
	// these are the random vectors inside a unit sphere
	vec3 pSphere[10] = {vec3(-0.010735935, 0.01647018, 0.0062425877),
                        vec3(-0.06533369, 0.3647007, -0.13746321),
                        vec3(-0.6539235, -0.016726388, -0.53000957),
                        vec3(0.40958285, 0.0052428036, -0.5591124),
                        vec3(-0.1465366, 0.09899267, 0.15571679),
                        vec3(-0.44122112, -0.5458797, 0.04912532),
                        vec3(0.03755566, -0.10961345, -0.33040273),
                        vec3(0.019100213, 0.29652783, 0.066237666),
                        vec3(0.8765323, 0.011236004, 0.28265962),
                        vec3(0.29264435, -0.40794238, 0.15964167)};
 
   // grab a normal for reflecting the sample rays later on
   vec3 fres = normalize((texture(rnm,_texCoord*offset).xyz*2.0) - vec3(1.0));
 
   vec4 currentPixelSample = texture(normalMap,_texCoord);
   currentPixelSample.w = 0.0;
   currentPixelSample = normalize(currentPixelSample);
   float currentPixelDepth = currentPixelSample.a;
 
   // current fragment coords in screen space
   vec3 ep = vec3(_texCoord,currentPixelDepth);
  // get the normal of current fragment
   vec3 norm = currentPixelSample.xyz;
 
   float bl = 0.0;
   // adjust for the depth ( not shure if this is good..)
   float radD = rad/currentPixelDepth;
 
   //vec3 ray, se, occNorm;
   float occluderDepth, depthDifference;
   vec4 occluderFragment;
   vec3 ray;

   for(int i=0; i < SAMPLES;++i)   {
      // get a vector (randomized inside of a sphere with radius 1.0) from a texture and reflect it
      ray = radD*reflect(pSphere[i],fres);
       // get the depth of the occluder fragment
      occluderFragment = texture(normalMap,ep.xy + sign(dot(ray,norm) )*ray.xy);
      // if depthDifference is negative = occluder is behind current fragment
      depthDifference = currentPixelDepth-occluderFragment.a;
      // calculate the difference between the normals as a weight
  	  // the falloff equation, starts at falloff and is kind of 1/x^2 falling
      bl += step(falloff,depthDifference)*(1.0-dot(occluderFragment.xyz,norm))*(1.0-smoothstep(falloff,strength,depthDifference));
   }

   // output the result
   _colorOut.r = 1.0+bl*invSamples;
}