#ifndef _FOLIAGE_VERT_
#define _FOLIAGE_VERT_

void computeFoliageMovementGrass(inout vec4 vertex, in float scaleFactor) {
    float timeGrass = dvd_windDetails.w * dvd_time * 0.001; //to seconds
    float cosX = cos(vertex.x);
    float sinX = sin(vertex.x);
    float halfScale = 0.5*scaleFactor;
    vertex.x += (halfScale*cos(timeGrass) * cosX * sinX)*dvd_windDetails.x;
    vertex.z += (halfScale*sin(timeGrass) * cosX * sinX)*dvd_windDetails.z;
}

#endif //_FOLIAGE_VERT_