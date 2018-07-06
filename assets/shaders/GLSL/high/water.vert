varying vec3 vPixToLight;		
varying vec3 vPixToEye;	
varying vec4 vPosition;
varying vec4 vVertexFromLightView;
varying vec4 texCoord[2];

uniform vec3 water_bb_min;
uniform vec3 water_bb_max;
uniform mat4 lightProjectionMatrix;

void main(void)
{
	gl_Position = ftransform();
	
	vPosition = gl_Vertex;
	vec3 vPositionNormalized = (gl_Vertex.xyz - water_bb_min.xyz) / (water_bb_max.xyz - water_bb_min.xyz);
	texCoord[0].st = vPositionNormalized.xz;
	
	vPixToLight = -(gl_LightSource[0].position.xyz);
	vPixToEye = -vec3(gl_ModelViewMatrix * gl_Vertex);	

	vVertexFromLightView = lightProjectionMatrix * gl_Vertex;	
	
}
