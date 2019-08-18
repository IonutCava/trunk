--Vertex

uniform float g_tileSize = 1.0f;

layout(location = ATTRIB_POSITION) in vec2 inPosition;
layout(location = ATTRIB_TEXCOORD) in vec4 inAdjacency;

layout(binding = TEXTURE_HEIGHT) uniform sampler2D TexTerrainHeight;

vec2 worldXZtoHeightUV(vec2 worldXZ)
{
    // [-8,8] -> [0,1]  TBD: Ought to depend on world size though.
    return worldXZ / 16 + 0.5;
}

float SampleHeightForVS(in vec2 uv)
{
    // You can implement purely procedural terrain here, evaluating the height fn on-the-fly.
    // But for any complex function, with loads of octaves, it's faster to sample from a texture.
    // return debugSineHills(uvCol);						// Simple test shape.
    // return 1.5 * hybridTerrain(uvCol, g_FractalOctaves);	// Procedurally sampled fractal terrain.

    // Fractal terrain sampled from texture.  This is a render target that is updated when the view 
    // point changes.  There are two levels of textures here.  1) The low-res ridge noise map that is 
    // initialized by the deformation effect.  2) A detail texture that contains 5 octaves of 
    // repeating fBm.
    float coarse = textureLod(TexTerrainHeight, uv, 0).r;	// coarse
    return (TERRAIN_HEIGHT_RANGE * coarse) + TERRAIN_MIN_HEIGHT;
}

void main(void)
{
    VAR.dvd_baseInstance = DATA_IDX;
    VAR.dvd_instanceID = gl_InstanceID;
    VAR.dvd_drawID = gl_DrawIDARB;

    // Send vertex position along
    float iv = floor(gl_VertexID * RECIP_CONTROL_VTX_PER_TILE_EDGE);
    float iu = gl_VertexID - iv * CONTROL_VTX_PER_TILE_EDGE;
    float u = iu / (CONTROL_VTX_PER_TILE_EDGE - 1.0f);
    float v = iv / (CONTROL_VTX_PER_TILE_EDGE - 1.0f);

    ivec2 intUV = ivec2(iu, iv);
    vec3 displacedPos = vec3(u * g_tileSize + inPosition.x, 0.0f, v * g_tileSize + inPosition.y);
    VAR._texCoord = worldXZtoHeightUV(displacedPos.xz);
    //displacedPos.y = SampleHeightForVS(VAR._texCoord);

    VAR._vertexW = inAdjacency;

    gl_Position = vec4(displacedPos, 1.0f);
}

--TessellationC

const float WORLD_SCALE = 1.0f;

// Most of the stuff here is from nVidia's DX11 terrain tessellation sample
#define id gl_InvocationID

//
// Outputs
//
layout(vertices = 4) out;
layout(location = 10) out float tcs_tessLevel[];

layout(binding = TEXTURE_HEIGHT) uniform sampler2D TexTerrainHeight;
uniform float tessTriangleWidth = 10.0f;

float saturate(in float val) { return clamp(val, 0.0f, 1.0f); }

// Lifted from Tim's Island demo code.
bool inFrustum(const vec3 pt, const vec3 eyePos, const vec3 viewDir, float margin)
{
    // conservative frustum culling
    vec3 eyeToPt = pt - eyePos;
    vec3 patch_to_camera_direction_vector = viewDir * dot(eyeToPt, viewDir) - eyeToPt;
    vec3 patch_center_realigned = pt + normalize(patch_to_camera_direction_vector) * min(margin, length(patch_to_camera_direction_vector));
    vec4 patch_screenspace_center = dvd_ProjectionMatrix * vec4(patch_center_realigned, 1.0);

    if (((patch_screenspace_center.x / patch_screenspace_center.w > -1.0) && (patch_screenspace_center.x / patch_screenspace_center.w < 1.0) &&
        (patch_screenspace_center.y / patch_screenspace_center.w > -1.0) && (patch_screenspace_center.y / patch_screenspace_center.w < 1.0) &&
        (patch_screenspace_center.w > 0)) || (length(pt - eyePos) < margin))
    {
        return true;
    }

    return false;
}

float getHeight(in vec2 tex_coord) {
    return (TERRAIN_HEIGHT_RANGE * texture(TexTerrainHeight, tex_coord).r) + TERRAIN_MIN_HEIGHT;
}

float ClipToScreenSpaceTessellation(vec4 clip0, vec4 clip1)
{
    clip0 /= clip0.w;
    clip1 /= clip1.w;

    clip0.xy *= dvd_ViewPort.zw;
    clip1.xy *= dvd_ViewPort.zw;

    const float d = distance(clip0, clip1);

    // g_tessellatedTriWidth is desired pixels per tri edge
    return clamp(d / tessTriangleWidth, 0, 64);
}

float SphereToScreenSpaceTessellation(vec3 p0, vec3 p1, in float diameter)
{
#if MAX_TESS_SCALE != MIN_TESS_SCALE
    vec3 center = 0.5f * (p0 + p1);
    vec4 view0 = dvd_ViewMatrix * vec4(center, 1.0f);
    vec4 view1 = view0;
    view1.x += WORLD_SCALE * diameter;

    vec4 clip0 = dvd_ProjectionMatrix * view0;
    vec4 clip1 = dvd_ProjectionMatrix * view1;
    return ClipToScreenSpaceTessellation(clip0, clip1);
#else
    return MAX_TESS_SCALE;
#endif
}


// The adjacency calculations ensure that neighbours have tessellations that agree.
// However, only power of two sizes *seem* to get correctly tessellated with no cracks.
float SmallerNeighbourAdjacencyClamp(float tess) {
    // Clamp to the nearest larger power of two.  Any power of two works; larger means that we don't lose detail.
    // Output is [4,64].
    float logTess = ceil(log2(tess));
    float t = pow(2, logTess);

    // Our smaller neighbour's min tessellation is pow(2,1) = 2.  As we are twice its size, we can't go below 4.
    return max(4, t);
}

float LargerNeighbourAdjacencyClamp(float tess) {
    // Clamp to the nearest larger power of two.  Any power of two works; larger means that we don't lose detail.
    float logTess = ceil(log2(tess));
    float t = pow(2, logTess);

    // Our larger neighbour's max tessellation is 64; as we are half its size, our tessellation must max out
    // at 32, otherwise we could be over-tessellated relative to the neighbour.  Output is [2,32].
    return clamp(t, 2, 32);
}

void MakeVertexHeightsAgree(inout vec3 p0, inout vec3 p1)
{
#if 0
    p0.y = p1.y = 0;
#else
    p0.y = getHeight(p0.xz);
    p1.y = getHeight(p1.xz);
#endif
}

float SmallerNeighbourAdjacencyFix(in int idx0, in int idx1, in float diameter) {
    vec3 p0 = gl_in[idx0].gl_Position.xyz;
    vec3 p1 = gl_in[idx1].gl_Position.xyz;

    MakeVertexHeightsAgree(p0, p1);
    float t = SphereToScreenSpaceTessellation(p0, p1, diameter);
    return SmallerNeighbourAdjacencyClamp(t);
}

float LargerNeighbourAdjacencyFix(in int idx0, in int idx1, in int patchIdx, in float diameter) {
    vec3 p0 = gl_in[idx0].gl_Position.xyz;
    vec3 p1 = gl_in[idx1].gl_Position.xyz;

    // We move one of the corner vertices in 2D (x,z) to match where the corner vertex is 
    // on our larger neighbour.  We move p0 or p1 depending on the even/odd patch index.
    //
    // Larger neighbour
    // +-------------------+
    // +---------+
    // p0   Us   p1 ---->  +		Move p1
    // |    0    |    1    |		patchIdx % 2 
    //
    //           +---------+
    // +  <----  p0   Us   p1		Move p0
    // |    0    |    1    |		patchIdx % 2 
    //
    if (patchIdx % 2 != 0)
        p0 += (p0 - p1);
    else
        p1 += (p1 - p0);

    // Having moved the vertex in (x,z), its height is no longer correct.
    MakeVertexHeightsAgree(p0, p1);

    // Half the tessellation because the edge is twice as long.
    float t = 0.5f * SphereToScreenSpaceTessellation(p0, p1, 2 * diameter);
    return LargerNeighbourAdjacencyClamp(t);
}

float getTessLevel(in int idx0, in int idx1, in float diameter) {
    vec3 p0 = gl_in[idx0].gl_Position.xyz;
    vec3 p1 = gl_in[idx1].gl_Position.xyz;

    return SphereToScreenSpaceTessellation(p0, p1, diameter);
}

#define neighbourMinusX x
#define neighbourMinusY y
#define neighbourPlusX  z
#define neighbourPlusY  w

void main(void)
{
    const vec4 adjacency = _in[0]._vertexW;

    const vec3  centre = 0.25 * (gl_in[0].gl_Position.xyz + gl_in[1].gl_Position.xyz + gl_in[2].gl_Position.xyz + gl_in[3].gl_Position.xyz);
    const float sideLen = max(abs(gl_in[1].gl_Position.x - gl_in[0].gl_Position.x), abs(gl_in[1].gl_Position.x - gl_in[2].gl_Position.x));		// assume square & uniform
    const float diagLen = sqrt(2 * sideLen * sideLen);
    //if (!inFrustum(centre, dvd_cameraPosition.xyz / WORLD_SCALE, dvd_cameraForward, diagLen))
    //{
    //    gl_TessLevelInner[0] = gl_TessLevelInner[1] = -1;
    //    gl_TessLevelOuter[0] = gl_TessLevelOuter[1] = -1;
    //    gl_TessLevelOuter[2] = gl_TessLevelOuter[3] = -1;
    //} 
    //else
    {
        PassData(id);

        // Outer tessellation level
        gl_TessLevelOuter[0] = 4;// getTessLevel(0, 1, sideLen);
        gl_TessLevelOuter[3] = 4;// getTessLevel(1, 2, sideLen);
        gl_TessLevelOuter[2] = 4;// getTessLevel(2, 3, sideLen);
        gl_TessLevelOuter[1] = 4;// getTessLevel(3, 0, sideLen);

        // Edges that need adjacency adjustment are identified by the per-instance ip[0].adjacency 
        // scalars, in *conjunction* with a patch ID that puts them on the edge of a tile.
        ivec2 patchXY;
        patchXY.y = gl_PrimitiveID / PATCHES_PER_TILE_EDGE;
        patchXY.x = gl_PrimitiveID - patchXY.y * PATCHES_PER_TILE_EDGE;

        // Identify patch edges that are adjacent to a patch of a different size.  The size difference
        // is encoded in _in[n].adjacency, either 0.5, 1.0 or 2.0.
        // neighbourMinusX refers to our adjacent neighbour in the direction of -ve x.  The value 
        // is the neighbour's size relative to ours.  Similarly for plus and Y, etc.  You really
        // need a diagram to make sense of the adjacency conditions in the if statements. :-(
        // These four ifs deal with neighbours that are smaller.
        /*if (adjacency.neighbourMinusX < 0.55 && patchXY.x == 0)
            gl_TessLevelOuter[0] = SmallerNeighbourAdjacencyFix(0, 1, sideLen);
        if (adjacency.neighbourMinusY < 0.55 && patchXY.y == 0)
            gl_TessLevelOuter[1] = SmallerNeighbourAdjacencyFix(3, 0, sideLen);
        if (adjacency.neighbourPlusX < 0.55 && patchXY.x == PATCHES_PER_TILE_EDGE - 1)
            gl_TessLevelOuter[2] = SmallerNeighbourAdjacencyFix(2, 3, sideLen);
        if (adjacency.neighbourPlusY < 0.55 && patchXY.y == PATCHES_PER_TILE_EDGE - 1)
            gl_TessLevelOuter[3] = SmallerNeighbourAdjacencyFix(1, 2, sideLen);

        // Deal with neighbours that are larger than us.
        if (adjacency.neighbourMinusX > 1 && patchXY.x == 0)
            gl_TessLevelOuter[0] = LargerNeighbourAdjacencyFix(0, 1, patchXY.y, sideLen);
        if (adjacency.neighbourMinusY > 1 && patchXY.y == 0)
            gl_TessLevelOuter[1] = LargerNeighbourAdjacencyFix(0, 3, patchXY.x, sideLen);	// NB: irregular index pattern - it's correct.
        if (adjacency.neighbourPlusX > 1 && patchXY.x == PATCHES_PER_TILE_EDGE - 1)
            gl_TessLevelOuter[2] = LargerNeighbourAdjacencyFix(3, 2, patchXY.y, sideLen);
        if (adjacency.neighbourPlusY > 1 && patchXY.y == PATCHES_PER_TILE_EDGE - 1)
            gl_TessLevelOuter[3] = LargerNeighbourAdjacencyFix(1, 2, patchXY.x, sideLen);	// NB: irregular index pattern - it's correct.*/

       // Inner tessellation level
       gl_TessLevelInner[1] = 0.5f * (gl_TessLevelOuter[0] + gl_TessLevelOuter[2]);
       gl_TessLevelInner[0] = 0.5f * (gl_TessLevelOuter[1] + gl_TessLevelOuter[3]);
       
    }

    // Pass the patch verts along
    gl_out[id].gl_Position = gl_in[id].gl_Position;
    // Output tessellation level (used for wireframe coloring)
    tcs_tessLevel[id] = gl_TessLevelOuter[0];
}

--TessellationE

layout(quads, fractional_even_spacing) in;

#include "nodeBufferedInput.cmn"
#include "waterData.cmn"

layout(binding = TEXTURE_HEIGHT) uniform sampler2D TexTerrainHeight;

layout(location = 10) in float tcs_tessLevel[];

#if defined(TOGGLE_WIREFRAME) || defined(TOGGLE_NORMALS)
layout(location = 10) out int tes_tessLevel;
#else
layout(location = 10) out flat int LoD;
// x = distance, y = depth
layout(location = 11) smooth out vec2 _waterDetails;
#endif

// Templates please!!!
vec2 Bilerp(vec2 v0, vec2 v1, vec2 v2, vec2 v3, vec2 i)
{
    vec2 v0Out = mix(v0, v1, i.x);
    vec2 v1Out = mix(v3, v2, i.x);
    return mix(v0Out, v1Out, i.y);
}

vec3 Bilerp(vec3 v0, vec3 v1, vec3 v2, vec3 v3, vec2 i)
{
    vec3 v0Out = mix(v0, v1, i.x);
    vec3 v1Out = mix(v3, v2, i.x);
    return mix(v0Out, v1Out, i.y);
}

vec4 getHeightOffsets(in vec2 tex_coord) {
    const ivec3 off = ivec3(-1, 0, 1);

    const float s01 = textureOffset(TexTerrainHeight, tex_coord, off.xy).r;
    const float s21 = textureOffset(TexTerrainHeight, tex_coord, off.zy).r;
    const float s10 = textureOffset(TexTerrainHeight, tex_coord, off.yx).r;
    const float s12 = textureOffset(TexTerrainHeight, tex_coord, off.yz).r;

    return (TERRAIN_HEIGHT_RANGE * vec4(s01, s21, s10, s12)) + TERRAIN_MIN_HEIGHT;
}

float getHeight(in vec4 heightOffsets) {
    
    const float s01 = heightOffsets.x;
    const float s21 = heightOffsets.y;
    const float s10 = heightOffsets.z;
    const float s12 = heightOffsets.w;

    return (s01 + s21 + s10 + s12) * 0.25f;
}

vec3 getNormal(in float sampleHeight, in vec4 heightOffsets) {
    float hL = heightOffsets.x;
    float hR = heightOffsets.y;
    float hD = heightOffsets.z;
    float hU = heightOffsets.w;

    // deduce terrain normal
    return normalize(vec3(hL - hR, 2.0f, hD - hU));
}

void main()
{
    PassData(0);

    // Terrain heightmap coords
    _out._texCoord = Bilerp(_in[0]._texCoord, _in[1]._texCoord, _in[2]._texCoord, _in[3]._texCoord, gl_TessCoord.xy);
    // Calculate the vertex position using the four original points and interpolate depending on the tessellation coordinates.	
    vec3 pos = Bilerp(gl_in[0].gl_Position.xyz, gl_in[1].gl_Position.xyz, gl_in[2].gl_Position.xyz, gl_in[3].gl_Position.xyz, gl_TessCoord.xy);
    pos.x *= 10;// TERRAIN_WIDTH;
    pos.z *= 10;// TERRAIN_LENGTH;

    // Sample the heightmap and offset y position of vertex
    const vec4 heightOffsets = getHeightOffsets(_out._texCoord);
    pos.y = 10.0f;// getHeight(heightOffsets);

    _out._vertexW = dvd_WorldMatrix(DATA_IDX) * vec4(pos, 1.0f);

#if !defined(SHADOW_PASS)
    const mat3 normalMatrixWV = dvd_NormalMatrixWV(DATA_IDX);

    const vec3 N = getNormal(pos.y, heightOffsets);
    _out._normalWV = normalize(normalMatrixWV * N);

#if defined(PRE_PASS)
    const vec3 B = cross(vec3(0.0f, 0.0f, 1.0f), N);
    const vec3 T = cross(N, B);

    _out._tbn = normalMatrixWV * mat3(T, B, N);
#endif

#endif
    _out._vertexWV = dvd_ViewMatrix * _out._vertexW;

#if defined(TOGGLE_WIREFRAME) || defined(TOGGLE_NORMALS)
    tes_tessLevel = int(tcs_tessLevel[0]);
    gl_Position = _out._vertexW;
#else
    gl_Position = dvd_ProjectionMatrix * _out._vertexWV;
    //setClipPlanes(_out._vertexW);

#if !defined(SHADOW_PASS)
#if !defined(TOGGLE_WIREFRAME) && !defined(TOGGLE_NORMALS)
    _waterDetails = waterDetails(_out._vertexW.xyz, TERRAIN_MIN_HEIGHT);
#endif

#if !defined(TOGGLE_WIREFRAME) || defined(TOGGLE_NORMALS)
    LoD = int(log2(MAX_TESS_SCALE / tcs_tessLevel[0]));
#endif //TOGGLE_WIREFRAME
#endif //SHADOW_PASS

#endif
}

--Geometry

#include "nodeBufferedInput.cmn"
#include "waterData.cmn"

layout(triangles) in;

layout(location = 10) in int tes_tessLevel[];


//#define SHOW_TILE_SCALE
#if defined(SHOW_TILE_SCALE)
struct TerrainNodeData {
    vec4 _positionAndTileScale;
    vec4 _tScale;
};

layout(binding = BUFFER_TERRAIN_DATA, std140) uniform dvd_TerrainBlock
{
    TerrainNodeData dvd_TerrainData[MAX_RENDER_NODES];
};
#endif

#if defined(TOGGLE_NORMALS)
layout(line_strip, max_vertices = 18) out;
#else
layout(triangle_strip, max_vertices = 4) out;
#endif

// x = distance, y = depth
layout(location = 10) out flat int LoD;
layout(location = 11) smooth out vec2 _waterDetails;

layout(location = 12) out vec3 gs_wireColor;
layout(location = 13) noperspective out vec3 gs_edgeDist;

vec4 getWVPPositon(in int i) {
    return dvd_ViewProjectionMatrix * gl_in[i].gl_Position;
}

void PerVertex(in int i, in vec3 edge_dist) {
    PassData(i);
    gl_Position = getWVPPositon(i);
#if !defined(SHADOW_PASS)
    LoD = int(log2(MAX_TESS_SCALE / tes_tessLevel[0]));

#if !defined(SHOW_TILE_SCALE)
    if (tes_tessLevel[0] == 64) {
        gs_wireColor = vec3(0.0, 0.0, 1.0);
    } else if (tes_tessLevel[0] == 32) {
        gs_wireColor = vec3(0.0, 1.0, 0.0);
    } else if (tes_tessLevel[0] == 16) {
        gs_wireColor = vec3(1.0, 0.0, 0.0);
    } else if (tes_tessLevel[0] == 8) {
        gs_wireColor = vec3(1.0, 1.0, 0.0);
    } else if (tes_tessLevel[0] == 4) {
        gs_wireColor = vec3(1.0, 0.0, 1.0);
    } else if (tes_tessLevel[0] == 2) {
        gs_wireColor = vec3(0.0, 1.00, 1.0);
    } else {
        gs_wireColor = vec3(1.0, 1.0, 1.0);
    }
#else
    const int tileScale = int(dvd_TerrainData[_in[i].dvd_instanceID]._positionAndTileScale.w);
    if (tileScale == 256) {
        gs_wireColor = vec3(0.0, 0.0, 1.0);
    }
    else if (tileScale == 128) {
        gs_wireColor = vec3(0.0, 1.0, 0.0);
    }
    else if (tileScale == 64) {
        gs_wireColor = vec3(1.0, 0.0, 0.0);
    }
    else if (tileScale == 32) {
        gs_wireColor = vec3(1.0, 1.0, 0.0);
    }
    else if (tileScale == 16) {
        gs_wireColor = vec3(1.0, 0.0, 1.0);
    }
    else if (tileScale == 8) {
        gs_wireColor = vec3(0.0, 1.00, 1.0);
    }
    else {
        gs_wireColor = vec3(1.0, 1.0, 1.0);
    }
#endif
    _waterDetails = waterDetails(VAR[i]._vertexW.xyz, TERRAIN_MIN_HEIGHT);

    gs_edgeDist = vec3(i == 0 ? edge_dist.x : 0.0,
                       i == 1 ? edge_dist.y : 0.0,
                       i >= 2 ? edge_dist.z : 0.0);
#endif
    //setClipPlanes(gl_in[i].gl_Position);
}

void main(void)
{
    // Calculate edge distances for wireframe
    vec3 edge_dist = vec3(0.0);
    {
        vec4 pos0 = getWVPPositon(0);
        vec4 pos1 = getWVPPositon(1);
        vec4 pos2 = getWVPPositon(2);

        vec2 p0 = vec2(dvd_ViewPort.zw * (pos0.xy / pos0.w));
        vec2 p1 = vec2(dvd_ViewPort.zw * (pos1.xy / pos1.w));
        vec2 p2 = vec2(dvd_ViewPort.zw * (pos2.xy / pos2.w));

        float a = length(p1 - p2);
        float b = length(p2 - p0);
        float c = length(p1 - p0);
        float alpha = acos((b * b + c * c - a * a) / (2.0 * b * c));
        float beta = acos((a * a + c * c - b * b) / (2.0 * a * c));
        edge_dist.x = abs(c * sin(beta));
        edge_dist.y = abs(c * sin(alpha));
        edge_dist.z = abs(b * sin(alpha));
    }

    const int count = gl_in.length();
#if defined(TOGGLE_NORMALS)

    const float sizeFactor = 0.75f;
    for (int i = 0; i < count; ++i) {

        mat3 normalMatrixWVInv = inverse(dvd_NormalMatrixWV(DATA_IDX));
        mat3 tbn = normalMatrixWVInv * _in[i]._tbn;

        vec3 P = gl_in[i].gl_Position.xyz;
        vec3 T = tbn[0];
        vec3 B = tbn[1];
        vec3 N = tbn[2];

        gs_edgeDist = vec3(i == 0 ? edge_dist.x : 0.0,
                           i == 1 ? edge_dist.y : 0.0,
                           i >= 2 ? edge_dist.z : 0.0);

        LoD = 0;
        // normals
        gs_wireColor = vec3(0.0f, 0.0f, 1.0f);
        gl_Position = dvd_ViewProjectionMatrix * vec4(P, 1.0);
        EmitVertex();

        gl_Position = dvd_ViewProjectionMatrix * vec4(P + N * sizeFactor, 1.0);
        EmitVertex();

        EndPrimitive();

        // binormals
        gs_wireColor = vec3(0.0f, 1.0f, 0.0f);
        gl_Position = dvd_ViewProjectionMatrix * vec4(P, 1.0);
        EmitVertex();

        gl_Position = dvd_ViewProjectionMatrix * vec4(P + B * sizeFactor, 1.0);
        EmitVertex();

        EndPrimitive();

        // tangents
        gs_wireColor = vec3(1.0f, 0.0f, 0.0f);
        gl_Position = dvd_ViewProjectionMatrix * vec4(P, 1.0);
        EmitVertex();

        gl_Position = dvd_ViewProjectionMatrix * vec4(P + T * sizeFactor, 1.0);
        EmitVertex();

        EndPrimitive();
    }
#else
    // Output verts
    for (int i = 0; i < count; ++i) {
        PerVertex(i, edge_dist);
        EmitVertex();
    }

    // This closes the triangle
    PerVertex(0, edge_dist);
    EmitVertex();

    EndPrimitive();
#endif
}

--Fragment.LQPass
layout(early_fragment_tests) in;

layout(location = 10) in flat int LoD;
layout(location = 11) smooth in vec2 _waterDetails;

#define NO_SPECULAR

#if defined(TOGGLE_WIREFRAME) || defined(TOGGLE_NORMALS)
layout(location = 12) in vec3 gs_wireColor;
layout(location = 13) noperspective in vec3 gs_edgeDist;
#endif

#define NEED_DEPTH_TEXTURE
#define USE_CUSTOM_ROUGHNESS
#include "BRDF.frag"
#include "output.frag"

#if defined(MAX_TEXTURE_LAYERS)
#undef MAX_TEXTURE_LAYERS
#define MAX_TEXTURE_LAYERS 1
#endif

#include "terrainSplatting.frag"


#if !defined(PRE_PASS)
vec2 getMetallicRoughness(in mat4 colourMatrix, in vec2 uv) {
    return vec2(0.2f, 0.8f);
}
#endif

void main(void)
{
    writeOutput(vec3(0.3f));
    return;

    TerrainData data = BuildTerrainData(_waterDetails);

    mat4 colourMatrix = dvd_Matrices[DATA_IDX]._colourMatrix;
    vec4 colourOut = getPixelColour(vec4(data.albedo.rgb, 1.0f), colourMatrix, data.normal, data.uv);

#if defined(TOGGLE_NORMALS)
    colourOut = vec4(gs_WireColor, 1.0f);
#elif defined(TOGGLE_WIREFRAME)
    const float LineWidth = 0.25f;
    float d = min(min(gs_edgeDist.x, gs_edgeDist.y), gs_edgeDist.z);
    colourOut = mix(vec4(gs_wireColor, 1.0f), colourOut, smoothstep(LineWidth - 1, LineWidth + 1, d));
#endif

    writeOutput(colourOut);
}

--Fragment.MainPass

#define NO_SPECULAR
#if !defined(PRE_PASS)
layout(early_fragment_tests) in;
#else
#define USE_CUSTOM_NORMAL_MAP
#endif

layout(location = 10) in flat int LoD;
// x = distance, y = depth
layout(location = 11) smooth in vec2 _waterDetails;

#define SHADOW_INTENSITY_FACTOR 0.75f

#if defined(TOGGLE_WIREFRAME) || defined(TOGGLE_NORMALS)
layout(location = 2) in vec3 gs_wireColor;
layout(location = 3) noperspective in vec3 gs_edgeDist;
#endif

#if defined(PRE_PASS)
#include "prePass.frag"
#else
#define NEED_DEPTH_TEXTURE
#define USE_CUSTOM_ROUGHNESS
#include "BRDF.frag"
#include "output.frag"
#endif

#include "terrainSplatting.frag"

#if !defined(PRE_PASS)
float _private_roughness = 0.0f;

vec2 getMetallicRoughness(in mat4 colourMatrix, in vec2 uv) {
    return vec2(0.2f, _private_roughness);
}
#endif

void main(void)
{

    TerrainData data = BuildTerrainData(_waterDetails);

#if defined(PRE_PASS)
    const float crtDepth = computeDepth(VAR._vertexWV);
    outputWithVelocity(data.uv, 1.0f, crtDepth, data.normal);
#else
    _private_roughness = data.albedo.a;

    mat4 colourMatrix = dvd_Matrices[DATA_IDX]._colourMatrix;
    vec4 colourOut = getPixelColour(vec4(data.albedo.rgb, 1.0f), colourMatrix, data.normal, data.uv);

#if defined(TOGGLE_WIREFRAME) || defined(TOGGLE_NORMALS)
    const float LineWidth = 0.75f;
    float d = min(min(gs_edgeDist.x, gs_edgeDist.y), gs_edgeDist.z);
    colourOut = mix(vec4(gs_wireColor, 1.0f), colourOut, smoothstep(LineWidth - 1, LineWidth + 1, d));
#endif

    writeOutput(colourOut);

#endif
}

--Fragment.Shadow

out vec2 _colourOut;

#include "nodeBufferedInput.cmn"
#include "vsm.frag"

void main() {

    _colourOut = computeMoments();
}
