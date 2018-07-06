varying vec3 normals;
varying float depth;

void main(void){
   gl_FragData[0] = vec4(normals.xyz,depth);
}