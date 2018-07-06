varying vec3         normals;
varying vec3         position;
uniform mat4         material;

void main( void )
{
   gl_FragData[0] = material[1]; //diffuse
   gl_FragData[1] = vec4(position,1);
   gl_FragData[2] = vec4(normals.xyz,0);
}