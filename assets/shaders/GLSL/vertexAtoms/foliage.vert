#ifndef _FOLIAGE_VERT_
#define _FOLIAGE_VERT_

uniform vec3  scale;
uniform float grassScale;
uniform float lod_metric;

void computeFoliageMovementTree(inout vec4 vertex) {

    float move_speed = (float(int(_vertexW.y*_vertexW.z) % 50)/50.0 + 0.5);
    float timeTree = dvd_time() * 0.001 * dvd_windDetails.w * move_speed; //to seconds
    float amplituted = pow(vertex.y, 2.0);
    vertex.x += 0.01 * amplituted * cos(timeTree + _vertexW.x) *dvd_windDetails.x;
    vertex.z += 0.05 *scale.y* amplituted * cos(timeTree + _vertexW.z) *dvd_windDetails.z;
}

void computeFoliageMovementGrass(inout vec4 vertex) {
    float timeGrass = dvd_windDetails.w * dvd_time() * 0.001; //to seconds
    float cosX = cos(vertex.x);
    float sinX = sin(vertex.x);
    float halfScale = 0.5*grassScale;
    vertex.x += (halfScale*cos(timeGrass) * cosX * sinX)*dvd_windDetails.x;
    vertex.z += (halfScale*sin(timeGrass) * cosX * sinX)*dvd_windDetails.z;
}

#endif //_FOLIAGE_VERT_