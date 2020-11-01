--Compute

// Ref:https://github.com/Angelo1211/HybridRenderingEngine/blob/master/assets/shaders/ComputeShaders/clusterCullLightShader.comp
// From blog post: http://www.aortiz.me/2018/12/21/CG.html#tiled-shading--forward

#include "lightInput.cmn"

//Function prototypes
vec4 screen2View(in vec4 screen);
vec3 lineIntersectionToZPlane(vec3 A, vec3 B, float zDistance);

layout(local_size_x = GRID_SIZE_X_THREADS, local_size_y = GRID_SIZE_Y_THREADS, local_size_z = GRID_SIZE_Z_THREADS) in;

void main() {
    //Shared between all clusters
    float zNear = dvd_zPlanes.x;
    float zFar = dvd_zPlanes.y;

    //Eye position is zero in view space
    const vec3 eyePos = vec3(0.0);

    //Per Tile variables
    uint clusterIndex = gl_GlobalInvocationID.z * gl_WorkGroupSize.x * gl_WorkGroupSize.y +
                        gl_GlobalInvocationID.y * gl_WorkGroupSize.x +
                        gl_GlobalInvocationID.x;

    //Calculating the min and max point in screen space
    vec4 minPoint_sS = vec4( gl_GlobalInvocationID.xy               * dvd_ClusterSizes, -1.0, 1.0); // Bottom left
    vec4 maxPoint_sS = vec4((gl_GlobalInvocationID.xy + vec2(1, 1)) * dvd_ClusterSizes, -1.0, 1.0); // Top Right

    //Pass min and max to view space
    vec3 maxPoint_vS = screen2View(maxPoint_sS).xyz;
    vec3 minPoint_vS = screen2View(minPoint_sS).xyz;

    //Near and far values of the cluster in view space
    float tileNear = -zNear * pow(zFar / zNear,  gl_GlobalInvocationID.z      / float(GRID_SIZE_Z));
    float tileFar  = -zNear * pow(zFar / zNear, (gl_GlobalInvocationID.z + 1) / float(GRID_SIZE_Z));

    //Finding the 4 intersection points made from the maxPoint to the cluster near/far plane
    vec3 minPointNear = lineIntersectionToZPlane(eyePos, minPoint_vS, tileNear);
    vec3 minPointFar  = lineIntersectionToZPlane(eyePos, minPoint_vS, tileFar);
    vec3 maxPointNear = lineIntersectionToZPlane(eyePos, maxPoint_vS, tileNear);
    vec3 maxPointFar  = lineIntersectionToZPlane(eyePos, maxPoint_vS, tileFar);

    vec3 minPointAABB = min(min(minPointNear, minPointFar), min(maxPointNear, maxPointFar));
    vec3 maxPointAABB = max(max(minPointNear, minPointFar), max(maxPointNear, maxPointFar));

    cluster[clusterIndex].minPoint = vec4(minPointAABB, 0.0);
    cluster[clusterIndex].maxPoint = vec4(maxPointAABB, 0.0);
}

//Creates a line from the eye to the screenpoint, then finds its intersection
//With a z oriented plane located at the given distance to the origin
vec3 lineIntersectionToZPlane(vec3 A, vec3 B, float zDistance) {
    //Because this is a Z based normal this is fixed
    const vec3 normal = vec3(0.0, 0.0, 1.0);

    const vec3 ab = B - A;

    //Computing the intersection length for the line and the plane
    const float t = (zDistance - dot(normal, A)) / dot(normal, ab);

    //Computing the actual xyz position of the point along the line
    return A + t * ab;
}

vec4 screen2View(in vec4 coord) {
    vec3 ndc = vec3(
        2.0 * (coord.x - dvd_ViewPort.x) / dvd_ViewPort.z - 1.0,
        2.0 * (coord.y - dvd_ViewPort.y) / dvd_ViewPort.w - 1.0,
        2.0 * coord.z - 1.0 // -> [-1, 1]
    );

    vec4 eye = dvd_InverseProjectionMatrix * vec4(ndc, 1.0);
    eye = eye / eye.w;

    return eye;
}