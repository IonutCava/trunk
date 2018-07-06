
uniform int win_width;
uniform int win_height;
uniform float noise_tile;
uniform float noise_factor;
uniform float time;
uniform float water_shininess;
uniform vec3 fog_color;

uniform sampler2D texWaterReflection;
uniform sampler2D texWaterNoiseNM;

varying vec3 vPixToLight;		// Vecteur du pixel courant à la lumière
varying vec3 vPixToEye;			// Vecteur du pixel courant à l'oeil
varying vec4 vPosition;

#define Z_TEST_SIGMA 0.0001
////////////////////


float Fresnel(vec3 incident, vec3 normal, float bias, float power);

void main (void)
{
	vec2 uvNormal0 = gl_TexCoord[0].st*noise_tile;
	uvNormal0.s += time*0.01;
	uvNormal0.t += time*0.01;
	vec2 uvNormal1 = gl_TexCoord[0].st*noise_tile;
	uvNormal1.s -= time*0.01;
	uvNormal1.t += time*0.01;
		
	vec3 normal0 = texture2D(texWaterNoiseNM, uvNormal0).rgb * 2.0 - 1.0;
	vec3 normal1 = texture2D(texWaterNoiseNM, uvNormal1).rgb * 2.0 - 1.0;
	vec3 normal = normalize(normal0+normal1);
	
	vec2 uvReflection = vec2(gl_FragCoord.x/win_width, gl_FragCoord.y/win_height);
	
	vec2 uvFinal = uvReflection.xy + noise_factor*normal.xy;
	gl_FragColor = texture2D(texWaterReflection, uvFinal);
	
	vec3 N = normalize(vec3(gl_ModelViewMatrix * vec4(normal.x, normal.z, normal.y, 0.0)));
	vec3 L = normalize(vPixToLight);
	vec3 V = normalize(vPixToEye);
	float iSpecular = pow(clamp(dot(reflect(-L, N), V), 0.0, 1.0), water_shininess);
	
	gl_FragColor += gl_LightSource[0].specular * iSpecular;
	gl_FragColor = clamp(gl_FragColor, vec4(0.0, 0.0, 0.0, 0.0),  vec4(1.0, 1.0, 1.0, 1.0));


	// FRESNEL ALPHA
	gl_FragColor.a	= Fresnel(V, N, 0.5, 2.0);

}


float Fresnel(vec3 incident, vec3 normal, float bias, float power)
{
	float scale = 1.0 - bias;
	return bias + pow(1.0 - dot(incident, normal), power) * scale;
}



