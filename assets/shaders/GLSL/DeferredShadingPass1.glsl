-- Vertex

#version 150 compatibility
varying vec3 normals;
varying vec3 position;
varying vec4 texCoord[2];

void main( void ){

	gl_Position =  gl_ModelViewProjectionMatrix * gl_Vertex;

	texCoord[0] = gl_MultiTexCoord0;
	position = vec3(transpose(gl_ModelViewMatrix) * gl_Vertex);
	normals = normalize(gl_NormalMatrix * gl_Normal);
} 

-- Fragment.NoTexture

varying vec3         normals;
varying vec3         position;
uniform mat4         material;

void main( void ){

	gl_FragData[0] = material[1]; //diffuse
    gl_FragData[1] = vec4(position,1);
    gl_FragData[2] = vec4(normals.xyz,1);
}

-- Fragment.Texture

varying vec3         normals;
varying vec3         position;
varying vec4 texCoord[2];

uniform sampler2D    texDiffuse0;

void main( void ){

   vec4 color = texture2D(texDiffuse0,texCoord[0].st);
   if(color.a < 0.2) discard;
   gl_FragData[0] = color;
   gl_FragData[1] = vec4(position,1);
   gl_FragData[2] = vec4(normals.xyz,1);
}

-- Fragment.Bump

#include "bumpMapping.frag"

void main( void ){
	
   vec4 color = texture2D(texDiffuse0,texCoord[0].st);
   if(color.a < 0.2) discard;
   gl_FragData[0] = color;
   gl_FragData[1] = vec4(position,1);
   gl_FragData[2] = vec4(normals.xyz,1);
}