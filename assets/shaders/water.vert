uniform vec3 water_min_depth;
uniform vec3 water_max_depth;

varying vec3 vPixToLight;		
varying vec3 vPixToEye;	
varying vec4 vPosition;
		
void main(void)
{
	gl_Position = ftransform();
	
	vPosition = gl_Vertex;
	vec3 vPositionNormalized = (gl_Vertex.xyz - water_min_depth.xyz) / (water_max_depth.xyz - water_min_depth.xyz);
	gl_TexCoord[0].st = vPositionNormalized.xz;
	
	vPixToLight = -(gl_LightSource[0].position.xyz);
	vPixToEye = -vec3(gl_ModelViewMatrix * gl_Vertex);	
	gl_TexCoord[1] = gl_TextureMatrix[0] * gl_Vertex;		
}
