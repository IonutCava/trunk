-- Vertex
attribute vec2 inTexCoordData;
attribute vec4 inColorData;

varying vec2 _texCoord;
uniform mat4 projectionMatrix;

void main(){
  _texCoord = inTexCoordData;
  gl_FrontColor = inColorData;
  gl_Position = projectionMatrix * gl_ModelViewMatrix * gl_Vertex;
} 


-- Fragment

uniform sampler2D texture;
uniform bool useTexture;
varying vec2 _texCoord;

void main(){
	if(!useTexture){
		gl_FragColor = gl_Color;
	}else{
		gl_FragColor = texture(texture, _texCoord)/* * gl_Color*/;
	}
}