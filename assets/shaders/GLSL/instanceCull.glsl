-- Vertex

layout(location = 10) in vec4 intanceData;
layout(location = 11) in float instanceData2;

uniform vec3 ObjectExtent;

uniform mat4 dvd_WorldViewMatrix;
uniform mat4 dvd_WorldViewProjectionMatrix;
uniform float dvd_visibilityDistance;

layout(binding = SHADER_BUFFER_CAM_MATRICES, std140) uniform dvd_MatrixBlock
{
    mat4 dvd_ProjectionMatrix;
    mat4 dvd_ViewMatrix;
    mat4 dvd_ViewProjectionMatrix;
    vec4 dvd_ViewPort;
};

/*layout(std430, binding = 10) buffer dvd_transformBlock{
    coherent readonly mat4 transform[];
};*/

out vec4 OrigData;
out float OrigData2;
flat out int OrigData3;
flat out int objectVisible;

vec4 BoundingBox[8];

uniform sampler2D HiZBuffer;

//subroutine int CullRoutineType(vec3 position);

//subroutine(CullRoutineType)
int PassThrough(vec3 position) {
    return 1;
}

//subroutine(CullRoutineType)
int InstanceCloudReduction(vec3 position) {
    if (distance(dvd_WorldViewMatrix * vec4(position, 1.0), vec4(0.0, 0.0, 0.0, 1.0)) > dvd_visibilityDistance) {
        return 0;
    }

    //mat4 finalTransform = dvd_WorldViewProjectionMatrix * transform[gl_InstanceID];
    mat4 finalTransform = dvd_WorldViewProjectionMatrix;
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
    int outOfBound[6] = int[6](0, 0, 0, 0, 0, 0);
    float limit = 0;
    for (int i = 0; i<8; i++)
    {
        if (BoundingBox[i].x >  BoundingBox[i].w) outOfBound[0]++;
        if (BoundingBox[i].x < -BoundingBox[i].w) outOfBound[1]++;
        if (BoundingBox[i].y >  BoundingBox[i].w) outOfBound[2]++;
        if (BoundingBox[i].y < -BoundingBox[i].w) outOfBound[3]++;
        if (BoundingBox[i].z >  BoundingBox[i].w) outOfBound[4]++;
        if (BoundingBox[i].z < -BoundingBox[i].w) outOfBound[5]++;
    }

    for (int i = 0; i < 6; i++)
        if (outOfBound[i] == 8)
            return 0;
    return 1;
}

//subroutine(CullRoutineType)
int HiZOcclusionCull(vec3 position) {
    /* first do instance cloud reduction */
    if (InstanceCloudReduction(position) == 0) return 0;

    /* perform perspective division for the bounding box */
    for (int i = 0; i<8; i++)
        BoundingBox[i].xyz /= BoundingBox[i].w;

    /* calculate screen space bounding rectangle */
    vec2 BoundingRect[2];
    BoundingRect[0].x = min(min(min(BoundingBox[0].x, BoundingBox[1].x), min(BoundingBox[2].x, BoundingBox[3].x)),
                            min(min(BoundingBox[4].x, BoundingBox[5].x), min(BoundingBox[6].x, BoundingBox[7].x))) / 2.0 + 0.5;

    BoundingRect[0].y = min(min(min(BoundingBox[0].y, BoundingBox[1].y), min(BoundingBox[2].y, BoundingBox[3].y)),
                            min(min(BoundingBox[4].y, BoundingBox[5].y), min(BoundingBox[6].y, BoundingBox[7].y))) / 2.0 + 0.5;

    BoundingRect[1].x = max(max(max(BoundingBox[0].x, BoundingBox[1].x), max(BoundingBox[2].x, BoundingBox[3].x)),
                            max(max(BoundingBox[4].x, BoundingBox[5].x), max(BoundingBox[6].x, BoundingBox[7].x))) / 2.0 + 0.5;

    BoundingRect[1].y = max(max(max(BoundingBox[0].y, BoundingBox[1].y), max(BoundingBox[2].y, BoundingBox[3].y)),
                            max(max(BoundingBox[4].y, BoundingBox[5].y), max(BoundingBox[6].y, BoundingBox[7].y))) / 2.0 + 0.5;

    /* then the linear depth value of the front-most point */
    float InstanceDepth = min(min(min(BoundingBox[0].z, BoundingBox[1].z), min(BoundingBox[2].z, BoundingBox[3].z)),
                              min(min(BoundingBox[4].z, BoundingBox[5].z), min(BoundingBox[6].z, BoundingBox[7].z)));

    /* now we calculate the bounding rectangle size in viewport coordinates */
    float ViewSizeX = (BoundingRect[1].x - BoundingRect[0].x) * dvd_ViewPort.y;
    float ViewSizeY = (BoundingRect[1].y - BoundingRect[0].y) * dvd_ViewPort.z;

    /* now we calculate the texture LOD used for lookup in the depth buffer texture */
    float LOD = ceil(log2(max(ViewSizeX, ViewSizeY) / 2.0));

    /* finally fetch the depth texture using explicit LOD lookups */
    vec4 Samples;
    Samples.x = textureLod(HiZBuffer, vec2(BoundingRect[0].x, BoundingRect[0].y), LOD).x;
    Samples.y = textureLod(HiZBuffer, vec2(BoundingRect[0].x, BoundingRect[1].y), LOD).x;
    Samples.z = textureLod(HiZBuffer, vec2(BoundingRect[1].x, BoundingRect[1].y), LOD).x;
    Samples.w = textureLod(HiZBuffer, vec2(BoundingRect[1].x, BoundingRect[0].y), LOD).x;
    float MaxDepth = max(max(Samples.x, Samples.y), max(Samples.z, Samples.w));

    /* if the instance depth is bigger than the depth in the texture discard the instance */
    return (InstanceDepth > MaxDepth) ? 0 : 1;
}

//subroutine uniform CullRoutineType CullRoutine;
uniform uint cullType = 0;

void main(void) {

    OrigData = intanceData;
    OrigData2 = instanceData2;
    OrigData3 = gl_InstanceID;
    //objectVisible = CullRoutine(OrigData.xyz);
    objectVisible = cullType != 0 ? HiZOcclusionCull(OrigData.xyz) : InstanceCloudReduction(OrigData.xyz);
}

-- Geometry

layout(points) in;
layout(points, max_vertices = 1) out;

in vec4 OrigData[1];
in float OrigData2[1];
flat in int OrigData3[1];

flat in int objectVisible[1];

out vec4 outVertexData;
out float outData1;
flat out int outData2;

//uniform uint queryId;

//layout(binding = 0, offset = 0) uniform atomic_uint primitiveCount[5];

void main() {

    // only emit primitive if the object is visible 
    if (objectVisible[0] == 1)  {
        //atomicCounterIncrement(primitiveCount[queryId]);
        outVertexData = OrigData[0];
        outData1 = OrigData2[0];
        outData2 = OrigData3[0];
        EmitVertex();
        EndPrimitive();
    }
}
