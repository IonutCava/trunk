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