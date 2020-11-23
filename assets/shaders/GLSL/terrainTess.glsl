--Vertex

#include "nodeBufferedInput.cmn"

layout(location = ATTRIB_POSITION) in vec4 inVertexData;
layout(location = ATTRIB_COLOR)    in vec4 inColourData;

#if !defined(USE_BINDLESS_TEXTURES)
layout(binding = TEXTURE_HEIGHT) uniform sampler2D texHeight;
#endif

uniform vec2 dvd_textureWorldOffset;
uniform vec2 dvd_tileWorldPosition;

layout(location = 10) out vec4 vtx_adjancency;
layout(location = 11) out float vtx_tileSize;
layout(location = 12) out flat int vtx_ringID;

vec2 worldXZtoHeightUV(vec2 worldXZ) {
    // Texture coords have to be offset by the eye's 2D world position.  Why the 2x???
    const vec2 uv = (worldXZ - dvd_textureWorldOffset) / vec2(UV_DIV_X, UV_DIV_Z);
    return saturate(0.5f * uv + 0.5f);
}

vec4 ReconstructPosition() {
#if defined(USE_BASE_VERTEX_OFFSET)
    const int vertID = gl_VertexID - gl_BaseVertex;
#else
    const int vertID = gl_VertexID;
#endif

    // Calculate texture coordinates (u,v) relative to entire terrain
    const float iv = floor(vertID * (1.0f / CONTROL_VTX_PER_TILE_EDGE));
    const float iu = vertID - iv * CONTROL_VTX_PER_TILE_EDGE;
    const float u = iu / (CONTROL_VTX_PER_TILE_EDGE - 1.0f);
    const float v = iv / (CONTROL_VTX_PER_TILE_EDGE - 1.0f);

    vtx_tileSize = inVertexData.z;
    vtx_adjancency = inColourData;
    vtx_ringID = int(inVertexData.w);

    vec2 tempPos = vec2(u * vtx_tileSize + inVertexData.x,
                        v * vtx_tileSize + inVertexData.y);

    tempPos.x *= WORLD_SCALE_X;
    tempPos.y *= WORLD_SCALE_Z;
    tempPos += 2 * vec2(dvd_tileWorldPosition.x, -dvd_tileWorldPosition.y);

    VAR._texCoord = tempPos;
    const vec2 texCoords = worldXZtoHeightUV(tempPos);
    const float height = (TERRAIN_HEIGHT * texture(texHeight, texCoords).r) + TERRAIN_HEIGHT_OFFSET;

    return vec4(tempPos.x, height, tempPos.y, 1.0f);
}

void main(void)
{
    const vec4 displacedPos = ReconstructPosition();
    _out._vertexW = dvd_Matrices[DATA_IDX]._worldMatrix * displacedPos;
    gl_Position = displacedPos;
}

--TessellationC

#include "nodeBufferedInput.cmn"

// Most of the stuff here is from nVidia's DX11 terrain tessellation sample
uniform float dvd_tessTriangleWidth;
uniform vec2 dvd_textureWorldOffset;

#if !defined(USE_BINDLESS_TEXTURES)
layout(binding = TEXTURE_HEIGHT) uniform sampler2D texHeight;
#endif

layout(location = 10) in vec4 vtx_adjancency[];
layout(location = 11) in float vtx_tileSize[];
layout(location = 12) in flat int vtx_ringID[];

// Outputs
layout(vertices = 4) out;
layout(location = 10) out float tcs_tileSize[];
layout(location = 11) out flat int tcs_ringID[];

#if defined(TOGGLE_DEBUG)
layout(location = 12) out vec3[5] tcs_debugColour[];
#if defined(TOGGLE_NORMALS)
#undef MAX_TESS_LEVEL
#define MAX_TESS_LEVEL 1
#endif //TOGGLE_NORMALS
#endif //TOGGLE_DEBUG

#if !defined(MAX_TESS_LEVEL)
#define MAX_TESS_LEVEL 64.0f
#endif //MAX_TESS_LEVEL

vec2 worldXZtoHeightUV(vec2 worldXZ) {
    const vec2 uv = (worldXZ - dvd_textureWorldOffset) / vec2(UV_DIV_X, UV_DIV_Z);
    return saturate(0.5f * uv + 0.5f);
}

float SphereToScreenSpaceTessellation(in vec3 w0, in vec3 w1, in float diameter)
{
    const vec3 centre = (w0 + w1) * 0.5f;

    const vec4 view0 = dvd_ViewMatrix * vec4(centre, 1.f);
    const vec4 view1 = view0 + vec4(diameter, 0.f, 0.f, 0.f);

    vec4 clip0 = dvd_ProjectionMatrix * view0; // to clip space
    vec4 clip1 = dvd_ProjectionMatrix * view1; // to clip space
    clip0 /= clip0.w;                          // project
    clip1 /= clip1.w;                          // project
    //clip0.xy = clip0.xy * 0.5f + 0.5f;       // to NDC (DX11 sample skipped this)
    //clip1.xy = clip1.xy * 0.5f + 0.5f;       // to NDC (DX11 sample skipped this)
    clip0.xy *= dvd_ViewPort.zw;               // to pixels
    clip1.xy *= dvd_ViewPort.zw;               // to pixels

    const float d = distance(clip0, clip1);
    return clamp(d / dvd_tessTriangleWidth, 1, MAX_TESS_LEVEL);
}


// The adjacency calculations ensure that neighbours have tessellations that agree.
// However, only power of two sizes *seem* to get correctly tessellated with no cracks.
float SmallerNeighbourAdjacencyClamp(in float tess) {
    // Clamp to the nearest larger power of two.  Any power of two works; larger means that we don't lose detail.
    // Output is [4, MAX_TESS_LEVEL]. Our smaller neighbour's min tessellation is pow(2,1) = 2.  As we are twice its size, we can't go below 4.
    const float t = pow(2, ceil(log2(tess)));
    return max(4, t);
}

float LargerNeighbourAdjacencyClamp(in float tess) {
    // Clamp to the nearest larger power of two.  Any power of two works; larger means that we don't lose detail.
    // Our larger neighbour's max tessellation is MAX_TESS_LEVEL; as we are half its size, our tessellation must max out
    // at MAX_TESS_LEVEL / 2, otherwise we could be over-tessellated relative to the neighbour.  Output is [2,MAX_TESS_LEVEL].
    const float t = pow(2, ceil(log2(tess)));
    return clamp(t, 2, MAX_TESS_LEVEL * 0.5f);
}

void MakeVertexHeightsAgree(in vec2 uv, inout vec3 p0, inout vec3 p1)
{
    p0.y = p1.y = (TERRAIN_HEIGHT * texture(texHeight, uv).r) + TERRAIN_HEIGHT_OFFSET;
}

float SmallerNeighbourAdjacencyFix(in int idx0, in int idx1, in float diameter) {
    vec3 p0 = _in[idx0]._vertexW.xyz;
    vec3 p1 = _in[idx1]._vertexW.xyz;
    const vec2 uv = worldXZtoHeightUV(gl_in[idx0].gl_Position.xz);

    MakeVertexHeightsAgree(uv, p0, p1);
    const float t = SphereToScreenSpaceTessellation(p0, p1, diameter);
    return SmallerNeighbourAdjacencyClamp(t);
}

float LargerNeighbourAdjacencyFix(in int idx0, in int idx1, in int patchIdx, in float diameter) {
    vec3 p0 = _in[idx0]._vertexW.xyz;
    vec3 p1 = _in[idx1]._vertexW.xyz;
    const vec2 uv = worldXZtoHeightUV(gl_in[idx0].gl_Position.xz);

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
    if (patchIdx % 2 != 0) {
        p0 += (p0 - p1);
    } else { 
        p1 += (p1 - p0);
    }

    // Having moved the vertex in (x,z), its height is no longer correct.
    MakeVertexHeightsAgree(uv, p0, p1);
    // Half the tessellation because the edge is twice as long.
    const float t = 0.5f * SphereToScreenSpaceTessellation(p0, p1, 2 * diameter);
    return LargerNeighbourAdjacencyClamp(t);
}

bool SphereInFrustum(vec3 pos, float r) {
    for (int i = 0; i < 6; i++) {
        if (dot(vec4(pos, 1.f), dvd_frustumPlanes[i]) + r < -0.25f) {
            // sphere outside plane
            return false;
        }
    }
    return true;
}

void main(void)
{
    const vec3   centre = 0.25f * (_in[0]._vertexW.xyz + _in[1]._vertexW.xyz + _in[2]._vertexW.xyz + _in[3]._vertexW.xyz);
    const float  radius = length(_in[0]._vertexW.xyz - _in[2]._vertexW.xyz) * 0.5f;
    if (!SphereInFrustum(centre, radius))
    {
          gl_TessLevelInner[0] = gl_TessLevelInner[1] = -1;
          gl_TessLevelOuter[0] = gl_TessLevelOuter[1] = -1;
          gl_TessLevelOuter[2] = gl_TessLevelOuter[3] = -1;
    }
    else
    {
        PassData(gl_InvocationID);

        const float sideLen = max(abs(_in[1]._vertexW.x - _in[0]._vertexW.x), abs(_in[1]._vertexW.x - _in[2]._vertexW.x));     // assume square & uniform

        // Outer tessellation level
        gl_TessLevelOuter[0] = SphereToScreenSpaceTessellation(_in[0]._vertexW.xyz, _in[1]._vertexW.xyz, sideLen);
        gl_TessLevelOuter[1] = SphereToScreenSpaceTessellation(_in[3]._vertexW.xyz, _in[0]._vertexW.xyz, sideLen);
        gl_TessLevelOuter[2] = SphereToScreenSpaceTessellation(_in[2]._vertexW.xyz, _in[3]._vertexW.xyz, sideLen);
        gl_TessLevelOuter[3] = SphereToScreenSpaceTessellation(_in[1]._vertexW.xyz, _in[2]._vertexW.xyz, sideLen);

#if !defined(LOW_QUALITY)
        // Edges that need adjacency adjustment are identified by the per-instance ip[0].adjacency 
        // scalars, in *conjunction* with a patch ID that puts them on the edge of a tile.
        const int PatchID = gl_PrimitiveID;
        ivec2 patchXY;
        patchXY.y = PatchID / PATCHES_PER_TILE_EDGE;
        patchXY.x = PatchID - patchXY.y * PATCHES_PER_TILE_EDGE;

        // Identify patch edges that are adjacent to a patch of a different size.  The size difference
        // is encoded in _in[n].adjacency, either 0.5, 1.0 or 2.0.
        // neighbourMinusX refers to our adjacent neighbour in the direction of -ve x.  The value
        // is the neighbour's size relative to ours.  Similarly for plus and Y, etc.  You really
        // need a diagram to make sense of the adjacency conditions in the if statements. :-(
        if (patchXY.x == 0) {
            if (vtx_adjancency[0].x < 0.55f) {
                // Deal with neighbours that are smaller.
                gl_TessLevelOuter[0] = SmallerNeighbourAdjacencyFix(0, 1, sideLen);
            } else if (vtx_adjancency[0].x > 1.1f) {
                // Deal with neighbours that are larger than us.
                gl_TessLevelOuter[0] = LargerNeighbourAdjacencyFix(0, 1, patchXY.y, sideLen);
            }
        } else if (patchXY.x == PATCHES_PER_TILE_EDGE - 1) {
            if (vtx_adjancency[0].z < 0.55f) {
                gl_TessLevelOuter[2] = SmallerNeighbourAdjacencyFix(2, 3, sideLen);
            } else if (vtx_adjancency[0].z > 1.1f) {
                gl_TessLevelOuter[2] = LargerNeighbourAdjacencyFix(3, 2, patchXY.y, sideLen);
            }
        }

        if (patchXY.y == 0) {
            if (vtx_adjancency[0].y < 0.55f) {
                gl_TessLevelOuter[1] = SmallerNeighbourAdjacencyFix(3, 0, sideLen);
            } else if (vtx_adjancency[0].y > 1.1f) {
                gl_TessLevelOuter[1] = LargerNeighbourAdjacencyFix(0, 3, patchXY.x, sideLen);	// NB: irregular index pattern - it's correct.
            }
        } else if (patchXY.y == PATCHES_PER_TILE_EDGE - 1) {
            if (vtx_adjancency[0].w < 0.55f) {
                gl_TessLevelOuter[3] = SmallerNeighbourAdjacencyFix(1, 2, sideLen);
            } else if (vtx_adjancency[0].w > 1.1f) {
                gl_TessLevelOuter[3] = LargerNeighbourAdjacencyFix(1, 2, patchXY.x, sideLen);	// NB: irregular index pattern - it's correct.
            }
        }
#endif //LOW_QUALITY

        // Inner tessellation level
        gl_TessLevelInner[0] = 0.5f * (gl_TessLevelOuter[0] + gl_TessLevelOuter[3]);
        gl_TessLevelInner[1] = 0.5f * (gl_TessLevelOuter[2] + gl_TessLevelOuter[1]);

        // Pass the patch verts along
        gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
        gl_out[gl_InvocationID].gl_Position.y = 0.0f;
        
        tcs_tileSize[gl_InvocationID] = vtx_tileSize[gl_InvocationID];
        tcs_ringID[gl_InvocationID] = vtx_ringID[gl_InvocationID];

#if defined(TOGGLE_DEBUG)
        // Output tessellation level (used for wireframe coloring)
        // These are one colour for each tessellation level and linear graduations between.
        const vec3 DEBUG_COLOURS[6] =
        {
            vec3(0,0,1), //  2 - blue   
            vec3(0,1,1), //  4 - cyan   
            vec3(0,1,0), //  8 - green  
            vec3(1,1,0), // 16 - yellow 
            vec3(1,0,1), // 32 - purple
            vec3(1,0,0), // 64 - red
        };

        tcs_debugColour[gl_InvocationID][0] = DEBUG_COLOURS[clamp(int(log2(gl_TessLevelOuter[0])), 0, 5)];
        tcs_debugColour[gl_InvocationID][1] = DEBUG_COLOURS[clamp(int(log2(gl_TessLevelOuter[1])), 0, 5)];
        tcs_debugColour[gl_InvocationID][2] = DEBUG_COLOURS[clamp(int(log2(gl_TessLevelOuter[2])), 0, 5)];
        tcs_debugColour[gl_InvocationID][3] = DEBUG_COLOURS[clamp(int(log2(gl_TessLevelOuter[3])), 0, 5)];
        tcs_debugColour[gl_InvocationID][4] = DEBUG_COLOURS[clamp(int(log2(gl_TessLevelInner[0])), 0, 5)];
#endif //TOGGLE_DEBUG
    }
}

--TessellationE

#if 0
#if defined(SHADOW_PASS)
layout(quads, fractional_even_spacing, ccw) in;
#else //SHADOW_PASS
layout(quads, fractional_even_spacing, cw) in;
#endif//SHADOW_PASS
#else
layout(quads, fractional_even_spacing, cw) in;
#endif

#include "nodeBufferedInput.cmn"

#if !defined(USE_BINDLESS_TEXTURES)
layout(binding = TEXTURE_HEIGHT) uniform sampler2D texHeight;
#endif

uniform vec2 dvd_textureWorldOffset;

layout(location = 10) in float tcs_tileSize[];
layout(location = 11) in flat int tcs_ringID[];

#if defined(TOGGLE_DEBUG)
layout(location = 12) in vec3[5] tcs_debugColour[];

layout(location = 10) out float tes_tileSize;
layout(location = 11) out flat int tes_ringID;
layout(location = 12) out vec3  tes_debugColour;
layout(location = 13) out float tes_PatternValue;

#else //TOGGLE_DEBUG

layout(location = 10) out flat int dvd_LoD;

#endif //TOGGLE_DEBUG

vec3 Bilerp(vec3 v0, vec3 v1, vec3 v2, vec3 v3, vec2 i)
{
    const vec3 bottom = lerp(v0, v3, i.x);
    const vec3 top = lerp(v1, v2, i.x);
    return lerp(bottom, top, i.y);
}

#if defined(TOGGLE_DEBUG)
vec3 BilerpColour(vec3 c0, vec3 c1, vec3 c2, vec3 c3, vec2 UV) {
    const vec3 left = lerp(c0, c1, UV.y);
    const vec3 right = lerp(c2, c3, UV.y);
    return lerp(left, right, UV.x);
}

vec3 LerpDebugColours(in vec3 cIn[5], vec2 uv) {
    if (uv.x < 0.5f && uv.y < 0.5f) {
        return BilerpColour(0.5f * (cIn[0] + cIn[1]), cIn[0], cIn[1], cIn[4], 2 * uv);
    } else if (uv.x < 0.5f && uv.y >= 0.5f) {
        return BilerpColour(cIn[0], 0.5f * (cIn[0] + cIn[3]), cIn[4], cIn[3], 2 * (uv - vec2(0.f, 0.5f)));
    } else if (uv.x >= 0.5f && uv.y < 0.5f) {
        return BilerpColour(cIn[1], cIn[4], 0.5f * (cIn[2] + cIn[1]), cIn[2], 2 * (uv - vec2(0.5f, 0.f)));
    } else {// x >= 0.5f && y >= 0.5f
        return BilerpColour(cIn[4], cIn[3], cIn[2], 0.5f * (cIn[2] + cIn[3]), 2 * (uv - vec2(0.5f, 0.5f)));
    }
}
#endif

float getHeight(in vec2 tex_coord) {
    return TERRAIN_HEIGHT * texture(texHeight, tex_coord).r + TERRAIN_HEIGHT_OFFSET;
}

vec2 worldXZtoHeightUV(in vec2 worldXZ) {
    const vec2 uv = (worldXZ - dvd_textureWorldOffset) / vec2(UV_DIV_X, UV_DIV_Z);
    return saturate(0.5f * uv + 0.5f);
}

#if !defined(PER_PIXEL_NORMALS)
vec3 getVertNormal(in vec2 tex_coord) {
    const ivec3 off = ivec3(-1, 0, 1);

    const float s01 = textureOffset(texHeight, tex_coord, off.xy).r; //-1,  0
    const float s21 = textureOffset(texHeight, tex_coord, off.zy).r; // 1,  0
    const float s10 = textureOffset(texHeight, tex_coord, off.yx).r; // 0, -1
    const float s12 = textureOffset(texHeight, tex_coord, off.yz).r; // 0,  1

    const float hL = TERRAIN_HEIGHT * s01 + TERRAIN_HEIGHT_OFFSET;
    const float hR = TERRAIN_HEIGHT * s21 + TERRAIN_HEIGHT_OFFSET;
    const float hD = TERRAIN_HEIGHT * s10 + TERRAIN_HEIGHT_OFFSET;
    const float hU = TERRAIN_HEIGHT * s12 + TERRAIN_HEIGHT_OFFSET;

    // deduce terrain normal
    return normalize(vec3(hL - hR, 2.0f, hD - hU));
}

void computeNormalData(in vec2 uv) {
    const vec3 N = getVertNormal(uv);
    _out._normalWV = mat3(dvd_ViewMatrix) * N;

#if !defined(USE_DEFERRED_NORMALS)
    _out._tbnViewDir = vec3(0.0f);
#else //!USE_DEFERRED_NORMALS
    const mat3 normalMatrix = NormalMatrixW(dvd_Matrices[DATA_IDX]);

    const vec3 B = cross(vec3(0.0f, 0.0f, 1.0f), N);
    const vec3 T = cross(N, B);

    const mat3 TBN = mat3(normalMatrix * T, normalMatrix * B, normalMatrix * N);
    _out._tbnWV = mat3(dvd_ViewMatrix) * TBN;
    _out._tbnViewDir = normalize(transpose(TBN) * (dvd_cameraPosition.xyz - _out._vertexW.xyz));
#endif//!USE_DEFERRED_NORMALS
}

#endif //PER_PIXEL_NORMALS

void main()
{
    PassData(0);
    // Calculate the vertex position using the four original points and interpolate depending on the tessellation coordinates.	
    const vec3 pos = Bilerp(gl_in[0].gl_Position.xyz,
                            gl_in[1].gl_Position.xyz,
                            gl_in[2].gl_Position.xyz,
                            gl_in[3].gl_Position.xyz,
                            gl_TessCoord.xy);

    _out._texCoord = worldXZtoHeightUV(pos.xz);
    _out._vertexW = dvd_Matrices[DATA_IDX]._worldMatrix * vec4(pos.x, getHeight(_out._texCoord), pos.z, 1.0f);
    setClipPlanes();
    
    _out._vertexWV = dvd_ViewMatrix * _out._vertexW;
    _out._vertexWVP = dvd_ProjectionMatrix * _out._vertexWV;

#if !defined(PER_PIXEL_NORMALS)
    computeNormalData(_out._texCoord);
#endif //PER_PIXEL_NORMALS

#if !defined(PRE_PASS) && !defined(SHADOW_PASS)
    _out._viewDirectionWV = mat3(dvd_ViewMatrix) * normalize(dvd_cameraPosition.xyz - _out._vertexW.xyz);
#endif //PRE_PASS && SHADOW_PASS

#if defined(TOGGLE_DEBUG)
    const int PatchID = gl_PrimitiveID;
    ivec2 patchXY;
    patchXY.y = PatchID / PATCHES_PER_TILE_EDGE;
    patchXY.x = PatchID - patchXY.y * PATCHES_PER_TILE_EDGE;

    tes_tileSize = tcs_tileSize[0];
    tes_ringID = tcs_ringID[0];
    tes_debugColour = LerpDebugColours(tcs_debugColour[0], gl_TessCoord.xy);
    tes_PatternValue = 0.5f * ((patchXY.x + patchXY.y) % 2);

    gl_Position = _out._vertexW;
#else //TOGGLE_DEBUG
    dvd_LoD = tcs_ringID[0];
    gl_Position = _out._vertexWVP;
#endif //TOGGLE_DEBUG
}

--Geometry

#include "nodeBufferedInput.cmn"

#if defined(TOGGLE_NORMALS)
#if !defined(USE_BINDLESS_TEXTURES)
layout(binding = TEXTURE_HEIGHT) uniform sampler2D texHeight;
#endif
#endif //TOGGLE_NORMALS

layout(triangles) in;

layout(location = 10) in float tes_tileSize[];
layout(location = 11) in flat int tes_ringID[];
layout(location = 12) in vec3 tes_debugColour[];
layout(location = 13) in float tes_PatternValue[];

layout(location = 10) out flat int dvd_LoD;
layout(location = 11) out float dvd_PatternValue;
layout(location = 12) out vec3 gs_wireColor;
layout(location = 13) noperspective out vec3 gs_edgeDist;

#if defined(TOGGLE_NORMALS)
layout(line_strip, max_vertices = 18) out;
#else //TOGGLE_NORMALS
layout(triangle_strip, max_vertices = 4) out;
#endif //TOGGLE_NORMALS

vec4 getWVPPositon(in int i) {
    return dvd_ViewProjectionMatrix * gl_in[i].gl_Position;
}

void PerVertex(in int i, in vec3 edge_dist) {
    PassData(i);
    gl_Position = getWVPPositon(i);

    dvd_LoD = tes_ringID[i];
    dvd_PatternValue = tes_PatternValue[i];
    gs_edgeDist = vec3(i == 0 ? edge_dist.x : 0.0,
                       i == 1 ? edge_dist.y : 0.0,
                       i >= 2 ? edge_dist.z : 0.0);
    setClipPlanes();
}

#if defined(TOGGLE_NORMALS)
vec3 getNormal(in vec2 tex_coord) {
    const ivec3 off = ivec3(-1, 0, 1);

    const float s01 = (TERRAIN_HEIGHT * textureOffset(texHeight, tex_coord, off.xy).r) + TERRAIN_HEIGHT_OFFSET;;
    const float s21 = (TERRAIN_HEIGHT * textureOffset(texHeight, tex_coord, off.zy).r) + TERRAIN_HEIGHT_OFFSET;;
    const float s10 = (TERRAIN_HEIGHT * textureOffset(texHeight, tex_coord, off.yx).r) + TERRAIN_HEIGHT_OFFSET;;
    const float s12 = (TERRAIN_HEIGHT * textureOffset(texHeight, tex_coord, off.yz).r) + TERRAIN_HEIGHT_OFFSET;;

    // deduce terrain normal
    return normalize(vec3(s01 - s21, 2.0f, s10 - s12));
}
#endif

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
    NodeData nodeData = dvd_Matrices[DATA_IDX];
    const float sizeFactor = 0.75f;
    for (int i = 0; i < count; ++i) {
        // In world space
        const vec3 N = getNormal(_in[i]._texCoord);
        const vec3 B = cross(vec3(0.0f, 0.0f, 1.0f), N);
        const vec3 T = cross(N, B);

        vec3 P = gl_in[i].gl_Position.xyz;
        { // normals
            PerVertex(0, edge_dist);
            gs_wireColor = vec3(0.0f, 0.0f, 1.0f);
            gl_Position = dvd_ViewProjectionMatrix * vec4(P, 1.0);
            EmitVertex();

            PerVertex(1, edge_dist);
            gs_wireColor = vec3(0.0f, 0.0f, 1.0f);
            gl_Position = dvd_ViewProjectionMatrix * vec4(P + N * sizeFactor, 1.0);
            EmitVertex();

            EndPrimitive();
        }
        { // binormals
            PerVertex(2, edge_dist);
            gs_wireColor = vec3(0.0f, 1.0f, 0.0f);
            gl_Position = dvd_ViewProjectionMatrix * vec4(P, 1.0);
            EmitVertex();

            PerVertex(3, edge_dist);
            gs_wireColor = vec3(0.0f, 1.0f, 0.0f);
            gl_Position = dvd_ViewProjectionMatrix * vec4(P + B * sizeFactor, 1.0);
            EmitVertex();

            EndPrimitive();
        }
        { // tangents
            PerVertex(4, edge_dist);
            gs_wireColor = vec3(1.0f, 0.0f, 0.0f);
            gl_Position = dvd_ViewProjectionMatrix * vec4(P, 1.0);
            EmitVertex();

            PerVertex(5, edge_dist);
            gs_wireColor = vec3(1.0f, 0.0f, 0.0f);
            gl_Position = dvd_ViewProjectionMatrix * vec4(P + T * sizeFactor, 1.0);
            EmitVertex();

            EndPrimitive();
        }
    }
#else //TOGGLE_NORMALS

    // Output verts
    for (int i = 0; i < count; ++i) {
        PerVertex(i, edge_dist);
        gs_wireColor = tes_debugColour[i];
        EmitVertex();
    }

    // This closes the triangle
    PerVertex(0, edge_dist);
    gs_wireColor = tes_debugColour[0];
    EmitVertex();

    EndPrimitive();

#endif //TOGGLE_NORMALS
}

--Fragment

#if !defined(PRE_PASS)
layout(early_fragment_tests) in;
#define USE_CUSTOM_ROUGHNESS
//#define SHADOW_INTENSITY_FACTOR 0.75f
#else //PRE_PASS
#define USE_CUSTOM_NORMAL_MAP
#endif //PRE_ASS

#define USE_CUSTOM_TBN
#define USE_CUSTOM_POM

layout(location = 10) in flat int dvd_LoD;

#if defined(TOGGLE_DEBUG)
layout(location = 11) in float dvd_PatternValue;
layout(location = 12) in vec3 gs_wireColor;
layout(location = 13) noperspective in vec3 gs_edgeDist;
#endif //TOGGLE_DEBUG

#if defined(LOW_QUALITY)
#if defined(MAX_TEXTURE_LAYERS)
#undef MAX_TEXTURE_LAYERS
#define MAX_TEXTURE_LAYERS 1
#endif //MAX_TEXTURE_LAYERS
#if defined(HAS_PARALLAX)
#undef HAS_PARALLAX
#endif //HAS_PARALLAX
#endif //LOW_QUALITY

#include "nodeBufferedInput.cmn"
#if defined(PRE_PASS)
#include "prePass.frag"
#if defined(HAS_PRE_PASS_DATA)
#include "terrainSplatting.frag"
#endif  //HAS_PRE_PASS_DATA
#else //PRE_PASS
#include "BRDF.frag"
#include "output.frag"
#include "terrainSplatting.frag"
#endif //PRE_PASS

#if !defined(PRE_PASS)
#if defined(LOW_QUALITY)

vec3 getOcclusionMetallicRoughness(in mat4 colourMatrix, in vec2 uv) {
    return vec3(0.0f, 0.0f, 0.8f);
}

#else //LOW_QUALITY

float _private_roughness = 0.0f;
vec3 getOcclusionMetallicRoughness(in mat4 colourMatrix, in vec2 uv) {
    return vec3(0.0f, 0.0f, _private_roughness);
}

#endif //LOW_QUALITY
#endif //PRE_PASS

void main(void)
{
#if defined(PRE_PASS)
#if defined(HAS_PRE_PASS_DATA)
    NodeData data = dvd_Matrices[DATA_IDX];
    prepareData(data);
    computeTBN(VAR._texCoord);

    writeOutput(data, 
                VAR._texCoord,
                getMixedNormalWV(VAR._texCoord));
#endif //HAS_PRE_PASS_DATA
#else //PRE_PASS

    NodeData data = dvd_Matrices[DATA_IDX];
    prepareData(data);

    computeTBN(VAR._texCoord);

    vec4 albedo;
    vec3 normalWV;

    BuildTerrainData(VAR._texCoord, albedo, normalWV);
#if !defined(LOW_QUALITY)
    _private_roughness = albedo.a;
#endif //LOW_QUALITY

    vec4 colourOut = getPixelColour(vec4(albedo.rgb, 1.0f), data, normalWV, VAR._texCoord);

#if defined (TOGGLE_DEBUG)

#if defined(TOGGLE_NORMALS)
    colourOut = vec4(gs_wireColor, 1.0f);
#else //TOGGLE_NORMALS
    float val = 0.5f * dvd_PatternValue;
    colourOut.rgb *= val;// vec4(vec3(val), 1.0f);

    const float LineWidth = 0.75f;
    const float d = min(min(gs_edgeDist.x, gs_edgeDist.y), gs_edgeDist.z);
    colourOut = mix(vec4(gs_wireColor, 1.0f), colourOut, smoothstep(LineWidth - 1, LineWidth + 1, d));
#endif //TOGGLE_NORMALS

#endif //TOGGLE_DEBUG

    writeOutput(colourOut);

#endif //PRE_PASS
}
