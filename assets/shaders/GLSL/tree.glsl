--Vertex

#include "vbInputData.vert"
#include "vegetationData.cmn"
#include "utility.cmn"

void computeFoliageMovementTree(inout vec4 vertex, in float heightExtent) {
    float time = dvd_windDetails.w * dvd_time * 0.00025f; //to seconds
    float cosX = cos(vertex.x);
    float sinX = sin(vertex.x);
    float halfScale = 0.5f * heightExtent;
    vertex.x += (halfScale * cos(time) * cosX * sinX) * dvd_windDetails.x;
    vertex.z += (halfScale * sin(time) * cosX * sinX) * dvd_windDetails.z;
}

void main(void){

    computeDataMinimal();

    const VegetationData data = TreeData(VAR.dvd_instanceID);

    float scale = data.positionAndScale.w;

    const float LoDValue = data.data.z;
#if defined(USE_CULL_DISTANCE)
    if (LoDValue > 2.1f) {
        gl_CullDistance[0] = -0.01f;
    }
#else
    scale -= scale * when_gt(LoDValue, 2.1f);
#endif


    dvd_Vertex.xyz = rotate_vertex_position(dvd_Vertex.xyz * scale, data.orientationQuad);
    if (LoDValue < 1.1f && dvd_Vertex.y * scale > 0.85f) {
        //computeFoliageMovementTree(dvd_Vertex, data.data.w);
    }

    VAR._vertexW = (dvd_Vertex + vec4(data.positionAndScale.xyz, 0.0f));

    VAR._vertexWV = dvd_ViewMatrix * VAR._vertexW;

#if !defined(SHADOW_PASS)
    VAR._normalWV = normalize(mat3(dvd_ViewMatrix) * dvd_Normal);
    setClipPlanes(VAR._vertexW);
#endif

    //Compute the final vert position
    gl_Position = dvd_ProjectionMatrix * VAR._vertexWV;
}