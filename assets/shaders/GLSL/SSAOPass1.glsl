-- Vertex
#include "vboInputData.vert"
out vec3 normals;
out float depth;

uniform vec2 dvd_zPlanes;

float LinearDepth(in float inDepth);

void main(void){
    computeData();
    normals = normalize(dvd_NormalMatrixWV() * dvd_Normal);
    vec4 vToEye = dvd_ModelViewMatrix * dvd_Vertex;    
    depth = LinearDepth(vToEye.z);
    gl_Position = dvd_ModelViewProjectionMatrix * dvd_Vertex;
}

float LinearDepth(in float inDepth){
    float dif = dvd_zPlanes.y - dvd_zPlanes.x;
    float A = -(dvd_zPlanes.y + dvd_zPlanes.x) / dif;
    float B = -2*dvd_zPlanes.y*dvd_zPlanes.x / dif;
    float C = -(A*inDepth + B) / inDepth; // C in [-1, 1]
    return 0.5 * C + 0.5; // in [0, 1]
}


-- Fragment

in vec3 normals;
in float depth;
out vec4 _colorOut;

void main(void){

   _colorOut = vec4(normalize(normals),depth);
}