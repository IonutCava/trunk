varying vec4 texCoord[2];
varying vec3 vVertexMV;

uniform float time;
uniform float lod_metric;
uniform float scale;
uniform float windDirectionX;
uniform float windDirectionZ;
uniform float windSpeed;
uniform int enable_shadow_mapping;
uniform mat4 modelViewInvMatrix;
uniform mat4 lightProjectionMatrix;

void main(void){
	
	texCoord[0] = gl_MultiTexCoord0;
	
	vec4 vertex = gl_Vertex;
	vec4 vertexMV = gl_ModelViewMatrix * gl_Vertex;
	vec3 normalMV = gl_NormalMatrix * gl_Normal;
	vVertexMV = vertexMV.xyz;

	if(gl_Normal.y < 0.0 ) {
		normalMV = -normalMV;
		vertex.x += ((0.5*scale)*cos(time*windSpeed) * cos(vertex.x) * sin(vertex.x))*windDirectionX;
		vertex.z += ((0.5*scale)*sin(time*windSpeed) * cos(vertex.x) * sin(vertex.x))*windDirectionZ;
	}
	
	vec4 vLightPosMV = -gl_LightSource[0].position;	
	float intensity = dot(vLightPosMV.xyz, normalMV);
	gl_FrontColor = vec4(intensity, intensity, intensity, 1.0);
	gl_FrontColor.a = 1.0 - clamp(length(vertexMV)/lod_metric, 0.0, 1.0);
		
	gl_Position = gl_ModelViewProjectionMatrix * vertex;
	
	if(enable_shadow_mapping != 0) {
		// Transformed position 
		vec4 pos = gl_ModelViewMatrix * gl_Vertex;
		// position multiplied by the inverse of the camera matrix
		pos = modelViewInvMatrix * pos;
		// position multiplied by the light matrix. 
		//The vertex's position from the light's perspective
		texCoord[1] = lightProjectionMatrix * pos;
	}
}





