-- Vertex

layout(location = 10) in vec4 intanceData;
layout(location = 11) in float instanceData2;

uniform vec3 ObjectExtent;
uniform float dvd_visibilityDistance;
uniform float dvd_frustumBias = 0.01;


/*layout(std430, binding = 10) coherent readonly buffer dvd_transformBlock{
    mat4 transform[];
};*/

out vec4 OrigData0;
out float OrigData1;
flat out int OrigData2;
flat out int objectVisible;

vec4 BoundingBox[8];

uniform sampler2D HiZBuffer;

//subroutine int CullRoutineType(const in vec3 position);
//subroutine uniform CullRoutineType CullRoutine;

//subroutine(CullRoutineType)
int PassThrough(const in vec3 position) {
    return 1;
}

//subroutine(CullRoutineType)
int InstanceCloudReduction(const in vec3 position) {

    //mat4 finalTransform = dvd_ViewProjectionMatrix * transform[gl_InstanceID];
    mat4 finalTransform = dvd_ViewProjectionMatrix;
    // create the bounding box of the object 
    BoundingBox[0] = finalTransform * vec4(position + vec3( ObjectExtent.x,  ObjectExtent.y,  ObjectExtent.z), 1.0);
    BoundingBox[1] = finalTransform * vec4(position + vec3(-ObjectExtent.x,  ObjectExtent.y,  ObjectExtent.z), 1.0);
    BoundingBox[2] = finalTransform * vec4(position + vec3( ObjectExtent.x, -ObjectExtent.y,  ObjectExtent.z), 1.0);
    BoundingBox[3] = finalTransform * vec4(position + vec3(-ObjectExtent.x, -ObjectExtent.y,  ObjectExtent.z), 1.0);
    BoundingBox[4] = finalTransform * vec4(position + vec3( ObjectExtent.x,  ObjectExtent.y, -ObjectExtent.z), 1.0);
    BoundingBox[5] = finalTransform * vec4(position + vec3(-ObjectExtent.x,  ObjectExtent.y, -ObjectExtent.z), 1.0);
    BoundingBox[6] = finalTransform * vec4(position + vec3( ObjectExtent.x, -ObjectExtent.y, -ObjectExtent.z), 1.0);
    BoundingBox[7] = finalTransform * vec4(position + vec3(-ObjectExtent.x, -ObjectExtent.y, -ObjectExtent.z), 1.0);

    // check how the bounding box resides regarding to the view frustum 
    uint outOfBound[6] = uint[6](0, 0, 0, 0, 0, 0);

    vec4 crtBB;
    float frustumLimit = 0.0;
    for (uint i = 0; i < 8; i++)  {
        crtBB = BoundingBox[i];
        frustumLimit = crtBB.w + dvd_frustumBias;
        if (crtBB.x >  frustumLimit) outOfBound[0]++;
        if (crtBB.x < -frustumLimit) outOfBound[1]++;
        if (crtBB.y >  frustumLimit) outOfBound[2]++;
        if (crtBB.y < -frustumLimit) outOfBound[3]++;
        if (crtBB.z >  frustumLimit) outOfBound[4]++;
        if (crtBB.z < -frustumLimit) outOfBound[5]++;
    }

    return (outOfBound[0] == 8 || outOfBound[1] == 8 || outOfBound[2] == 8 ||
            outOfBound[3] == 8 || outOfBound[4] == 8 || outOfBound[5] == 8) ? 0 : 1;
}

//subroutine(CullRoutineType)
int HiZOcclusionCull(const in vec3 position) {
    /* first do instance cloud reduction */
    if (InstanceCloudReduction(position) == 0) {
        return 0;
    }

    if (distance(dvd_ViewMatrix * vec4(position, 1.0), vec4(0.0, 0.0, 0.0, 1.0)) > dvd_visibilityDistance) {
        return 0;
    }

    /* perform perspective division for the bounding box */
    BoundingBox[0].xyz /= BoundingBox[0].w;
    BoundingBox[1].xyz /= BoundingBox[1].w;
    BoundingBox[2].xyz /= BoundingBox[2].w;
    BoundingBox[3].xyz /= BoundingBox[3].w;
    BoundingBox[4].xyz /= BoundingBox[4].w;
    BoundingBox[5].xyz /= BoundingBox[5].w;
    BoundingBox[6].xyz /= BoundingBox[6].w;
    BoundingBox[7].xyz /= BoundingBox[7].w;

    /* calculate screen space bounding rectangle */
    vec2 BoundingRect[2];
    BoundingRect[0].x = min(min(min(BoundingBox[0].x, BoundingBox[1].x), min(BoundingBox[2].x, BoundingBox[3].x)),
                            min(min(BoundingBox[4].x, BoundingBox[5].x), min(BoundingBox[6].x, BoundingBox[7].x))) * 0.5 + 0.5;

    BoundingRect[0].y = min(min(min(BoundingBox[0].y, BoundingBox[1].y), min(BoundingBox[2].y, BoundingBox[3].y)),
                            min(min(BoundingBox[4].y, BoundingBox[5].y), min(BoundingBox[6].y, BoundingBox[7].y))) * 0.5 + 0.5;

    BoundingRect[1].x = max(max(max(BoundingBox[0].x, BoundingBox[1].x), max(BoundingBox[2].x, BoundingBox[3].x)),
                            max(max(BoundingBox[4].x, BoundingBox[5].x), max(BoundingBox[6].x, BoundingBox[7].x))) * 0.5 + 0.5;

    BoundingRect[1].y = max(max(max(BoundingBox[0].y, BoundingBox[1].y), max(BoundingBox[2].y, BoundingBox[3].y)),
                            max(max(BoundingBox[4].y, BoundingBox[5].y), max(BoundingBox[6].y, BoundingBox[7].y))) * 0.5 + 0.5;

    /* then the linear depth value of the front-most point */
    float InstanceDepth = min(min(min(BoundingBox[0].z, BoundingBox[1].z), min(BoundingBox[2].z, BoundingBox[3].z)),
                              min(min(BoundingBox[4].z, BoundingBox[5].z), min(BoundingBox[6].z, BoundingBox[7].z)));

    /* now we calculate the bounding rectangle size in viewport coordinates */
    /* now we calculate the texture LOD used for lookup in the depth buffer texture */
    float LOD = ceil(log2(max((BoundingRect[1].x - BoundingRect[0].x) * dvd_ViewPort.y, (BoundingRect[1].y - BoundingRect[0].y) * dvd_ViewPort.z) / 2.0));

    /* finally fetch the depth texture using explicit LOD lookups */
    /* if the instance depth is bigger than the depth in the texture discard the instance */
    return (InstanceDepth > max(max(textureLod(HiZBuffer, vec2(BoundingRect[0].x, BoundingRect[0].y), LOD).x, 
                                    textureLod(HiZBuffer, vec2(BoundingRect[0].x, BoundingRect[1].y), LOD).x),
                                max(textureLod(HiZBuffer, vec2(BoundingRect[1].x, BoundingRect[1].y), LOD).x,
                                    textureLod(HiZBuffer, vec2(BoundingRect[1].x, BoundingRect[0].y), LOD).x))) ? 0 : 1;
}

uniform uint cullType = 0;

void main(void) {

    OrigData0 = intanceData;
    OrigData1 = instanceData2;
    OrigData2 = gl_InstanceID;
    //objectVisible = CullRoutine(OrigData.xyz);
    objectVisible = cullType > 0 ? (cullType > 1 ? HiZOcclusionCull(OrigData0.xyz) : InstanceCloudReduction(OrigData0.xyz)) : PassThrough(OrigData0.xyz);
}

-- Geometry

layout(points) in;
layout(points, max_vertices = 1) out;

in vec4 OrigData0[1];
in float OrigData1[1];
flat in int OrigData2[1];

flat in int objectVisible[1];

out vec4 outData0;
out float outData1;
flat out int outData2;

//uniform uint queryId;
//layout(binding = 0, offset = 0) uniform atomic_uint primitiveCount[5];

void main() {

    // only emit primitive if the object is visible 
    if (objectVisible[0] == 1)  {
        //atomicCounterIncrement(primitiveCount[queryId]);
        outData0 = OrigData0[0];
        outData1 = OrigData1[0];
        outData2 = OrigData2[0];
        EmitVertex();
        EndPrimitive();
    }
}
