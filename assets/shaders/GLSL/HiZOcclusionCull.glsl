-- Compute

//ref: http://malideveloper.arm.com/resources/sample-code/occlusion-culling-hierarchical-z/
struct NodeData {
    mat4 _matrix[4];
    vec4 _boundingSphere;
};

layout(binding = BUFFER_NODE_INFO, std430) buffer dvd_MatrixBlock
{
    readonly NodeData dvd_Matrices[MAX_VISIBLE_NODES + 1];
};

layout(location = 0) uniform uint dvd_numEntities;
layout(binding = TEXTURE_DEPTH_MAP) uniform sampler2D texDepth;
layout(binding = 0, offset = 0) uniform atomic_uint culledCount;

layout(local_size_x = 64) in;
void main()
{
    uint ident = gl_GlobalInvocationID.x;
    if (ident >= dvd_numEntities) {
        return;
    }
    
    uint nodeIndex = dvd_drawCommands[ident].baseInstance;

    vec3 center = dvd_Matrices[nodeIndex]._boundingSphere.xyz;
    float radius = dvd_Matrices[nodeIndex]._boundingSphere.w;

    vec3 view_center = (dvd_ViewMatrix * vec4(center, 1.0)).xyz;
    float nearest_z = view_center.z + radius;

    // Sphere clips against near plane, just assume visibility.
    if (nearest_z >= -dvd_ZPlanesCombined.x)
    {
        return;
    }

    float az_plane_horiz_length = length(view_center.xz);
    float az_plane_vert_length = length(view_center.yz);
    vec2 az_plane_horiz_norm = view_center.xz / az_plane_horiz_length;
    vec2 az_plane_vert_norm = view_center.yz / az_plane_vert_length;

    vec2 t = sqrt(vec2(az_plane_horiz_length, az_plane_vert_length) * vec2(az_plane_horiz_length, az_plane_vert_length) - radius * radius);
    vec4 w = vec4(t, radius, radius) / vec2(az_plane_horiz_length, az_plane_vert_length).xyxy;

    vec4 horiz_cos_sin = az_plane_horiz_norm.xyyx * t.x * vec4(w.xx, -w.z, w.z);
    vec4 vert_cos_sin = az_plane_vert_norm.xyyx * t.y * vec4(w.yy, -w.w, w.w);

    vec2 horiz0 = horiz_cos_sin.xy + horiz_cos_sin.zw;
    vec2 horiz1 = horiz_cos_sin.xy - horiz_cos_sin.zw;
    vec2 vert0 = vert_cos_sin.xy + vert_cos_sin.zw;
    vec2 vert1 = vert_cos_sin.xy - vert_cos_sin.zw;

    vec4 projected = -0.5 * vec4(dvd_ProjectionMatrix[0][0], dvd_ProjectionMatrix[0][0], dvd_ProjectionMatrix[1][1], dvd_ProjectionMatrix[1][1]) *
                            vec4(horiz0.x, horiz1.x, vert0.x, vert1.x) /
                            vec4(horiz0.y, horiz1.y, vert0.y, vert1.y) + 0.5;

    vec2 min_xy = projected.yw;
    vec2 max_xy = projected.xz;

    vec2 zw = mat2(dvd_ProjectionMatrix[2].zw, dvd_ProjectionMatrix[3].zw) * vec2(nearest_z, 1.0);
    nearest_z = 0.5 * zw.x / zw.y + 0.5;

    vec2 diff_pix = (max_xy - min_xy) * vec2(textureSize(texDepth, 0));
    float max_diff = max(max(diff_pix.x, diff_pix.y), 1.0);
    float lod = ceil(log2(max_diff));
    vec2 mid_pix = 0.5 * (max_xy + min_xy);

    if (textureLod(texDepth, mid_pix, lod).x > 0.0) {
        return;
    }

    atomicCounterIncrement(culledCount);
    dvd_drawCommands[ident].instanceCount = 0;
}