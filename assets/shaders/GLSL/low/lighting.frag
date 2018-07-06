varying vec2 texCoord;

//Textures
uniform int textureCount;
uniform bool hasOpacity;
uniform sampler2D texDiffuse0;
uniform sampler2D texDiffuse1;
uniform sampler2D opacityMap;
uniform sampler2D specularMap;
//Material properties
uniform mat4 material;

uniform float tile_factor;

void main (void){

	vec2 uv = texCoord*tile_factor;
	if(hasOpacity){
		vec4 alpha = texture2D(opacityMap, uv);
		if(alpha.a < 0.2) discard;
	}

	vec4 cAmbient = gl_LightSource[0].ambient * material[0];
	vec4 cDiffuse = gl_LightSource[0].diffuse * material[1];	
	vec4 cSpecular = gl_LightSource[0].specular * material[2] ;	
		
	vec4 tBase[2];
	tBase[0]  = vec4(1.0,1.0,1.0,1.0);
	if(textureCount > 0){
		tBase[0] = texture2D(texDiffuse0, uv);
		if(tBase[0].a < 0.4) discard;
		if(textureCount > 1){
			tBase[1] = texture2D(texDiffuse1,uv);
			tBase[0] = tBase[0] + tBase[1];
		} 
	}

	vec4 color = cAmbient  * tBase[0] + (cDiffuse   * tBase[0] + cSpecular);
	if(color.a < 0.2) discard;
	gl_FragColor = color;


}