-- Vertex

#include "HiZCullingAlgorithm.cmn";

layout(location = 10) in vec4 intanceData;
layout(location = 11) in float instanceData2;

uniform vec3 ObjectExtent;
uniform float dvd_visibilityDistance;

/*layout(std430, binding = 10) coherent readonly buffer dvd_transformBlock{
    mat4 transform[];
};*/

out vec4 OrigData0;
out float OrigData1;
flat out int OrigData2;
flat out int objectVisible;

uniform uint cullType = 0;

int occlusionCull() {
    if (distance(dvd_ViewMatrix * vec4(position, 1.0), vec4(0.0, 0.0, 0.0, 1.0)) > dvd_visibilityDistance) {
        return 0;
    }

    switch (cullType) {
        case 0 :
            return PassThrough(OrigData0.xyz, ObjectExtent);
        case 1 : 
            return InstanceCloudReduction(OrigData0.xyz, ObjectExtent);
    };

    return HiZOcclusionCull(OrigData0.xyz, ObjectExtent);
                           
}

void main(void) {

    OrigData0 = intanceData;
    OrigData1 = instanceData2;
    OrigData2 = gl_InstanceID;
    objectVisible = occlusionCull();
}

-- Geometry

layout(points) in;
layout(points, max_vertices = 1) out;

in vec4 OrigData0[1];
in float OrigData1[1];
flat in int OrigData2[1];

flat in int objectVisible[1];

layout (xfb_buffer=0) out vec4 outData0;
layout (xfb_buffer=1) out float outData1;
layout (xfb_buffer=2) flat out int outData2;

//uniform uint queryID;
//layout(binding = 0, offset = 0) uniform atomic_uint primitiveCount[5];

void main() {

    // only emit primitive if the object is visible 
    if (objectVisible[0] == 1)  {
        //atomicCounterIncrement(primitiveCount[queryID]);
        outData0 = OrigData0[0];
        outData1 = OrigData1[0];
        outData2 = OrigData2[0];
        EmitVertex();
        EndPrimitive();
    }
}
