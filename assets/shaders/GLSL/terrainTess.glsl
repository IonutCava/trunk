--Vertex

#include "vbInputData.vert"

layout(binding = TEXTURE_HEIGHT) uniform sampler2D TexTerrainHeight;

layout(location = 10) out flat uint dvd_instanceID;

struct TerrainNodeData {
    vec4 _positionAndTileScale;
    vec4 _tScale;
};

layout(binding = BUFFER_TERRAIN_DATA, std430) coherent readonly buffer dvd_TerrainBlock
{
    TerrainNodeData dvd_TerrainData[];
};

void main(void)
{
    VAR._baseInstance = DATA_IDX;
    dvd_instanceID = gl_InstanceID;

    const TerrainNodeData tData = dvd_TerrainData[dvd_instanceID];

    // Calculate texture coordinates (u,v) relative to entire terrain
    const float iv = floor(gl_VertexID * (1.0f / CONTROL_VTX_PER_TILE_EDGE));
    const float iu = gl_VertexID - iv * CONTROL_VTX_PER_TILE_EDGE;
    const float u = 2.0f * (iu / (CONTROL_VTX_PER_TILE_EDGE - 1.0f)) - 1.0f;
    const float v = 2.0f * (iv / (CONTROL_VTX_PER_TILE_EDGE - 1.0f)) - 1.0f;

    dvd_Vertex = vec4(u, 0.f, v, 1.f);
    dvd_Vertex.xyz *= int(tData._positionAndTileScale.w);
    dvd_Vertex.xyz += tData._positionAndTileScale.xyz;
    VAR._texCoord = vec2(abs(dvd_Vertex.x - TERRAIN_ORIGIN_X) / TERRAIN_WIDTH,
                         abs(dvd_Vertex.z - TERRAIN_ORIGIN_Y) / TERRAIN_LENGTH);
    VAR._vertexW = dvd_Vertex;
    VAR._vertexW.y = (TERRAIN_HEIGHT_RANGE * texture(TexTerrainHeight, VAR._texCoord).r) + TERRAIN_MIN_HEIGHT;

    // Send vertex position along
    gl_Position = VAR._vertexW;
}

--TessellationC

// Most of the stuff here is from nVidia's DX11 terrain tessellation sample
uniform float tessTriangleWidth;

layout(binding = TEXTURE_HEIGHT) uniform sampler2D TexTerrainHeight;

struct TerrainNodeData
{
    vec4 _positionAndTileScale;
    vec4 _tScale;
};

layout(binding = BUFFER_TERRAIN_DATA, std430) coherent readonly buffer dvd_TerrainBlock {
    TerrainNodeData dvd_TerrainData[];
};

layout(location = 10) in flat uint dvd_instanceID[];
// Outputs
layout(vertices = 4) out;
layout(location = 10) out float tcs_tessLevel[];
layout(location = 11) out int tcs_tileScale[];

// Lifted from Tim's Island demo code.
bool inFrustum(in vec3 pt, in vec3 eyePos, in vec3 viewDir, in float margin)
{
    // conservative frustum culling
    const vec3 eyeToPt = pt - eyePos;
    const vec3 patch_to_camera_direction_vector = viewDir * dot(eyeToPt, viewDir) - eyeToPt;
    const vec3 patch_center_realigned = pt + normalize(patch_to_camera_direction_vector) * min(margin, length(patch_to_camera_direction_vector));
    const vec4 patch_screenspace_center = dvd_ProjectionMatrix * vec4(patch_center_realigned, 1.0);

    const float xDw = patch_screenspace_center.x / patch_screenspace_center.w;
    const float yDw = patch_screenspace_center.y / patch_screenspace_center.w;
    return (((xDw > -1.0f) && (xDw < 1.0f) &&
             (yDw > -1.0f) && (yDw < 1.0f) &&
             (patch_screenspace_center.w > 0.0f)) || (length(eyeToPt) >= 0.0f));
}

float SphereToScreenSpaceTessellation(in vec3 p0, in vec3 p1, in float diameter)
{
    const vec3 center = 0.5f * (p0 + p1);
    vec4 view0 = dvd_ViewMatrix * vec4(center, 1.0f);
    vec4 view1 = view0;
    view1.x += diameter;

    vec4 clip0 = dvd_ProjectionMatrix * view0;
    vec4 clip1 = dvd_ProjectionMatrix * view1;

    clip0 /= clip0.w;
    clip1 /= clip1.w;

    clip0.xy *= dvd_ViewPort.zw;
    clip1.xy *= dvd_ViewPort.zw;
    const float d = distance(clip0, clip1);

    // tessTriangleWidth is desired pixels per tri edge
    return clamp(d / tessTriangleWidth, 0.f, MAX_TESS_LEVEL);
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

void MakeVertexHeightsAgree(inout vec3 p0, inout vec3 p1)
{
    const vec2 texCoords = vec2(abs(p0.x - TERRAIN_ORIGIN_X) / TERRAIN_WIDTH,
                                abs(p0.y - TERRAIN_ORIGIN_Y) / TERRAIN_LENGTH);
    // This ought to work: if the adjacency has repositioned a vertex in XZ, we need to re-acquire its height.
    // However, causes an internal fxc error.  Again! :-(
    p0.y = p1.y = (TERRAIN_HEIGHT_RANGE * texture(TexTerrainHeight, texCoords).r) + TERRAIN_MIN_HEIGHT;
}

float SmallerNeighbourAdjacencyFix(in int idx0, in int idx1, in float diameter) {
    vec3 p0 = gl_in[idx0].gl_Position.xyz;
    vec3 p1 = gl_in[idx1].gl_Position.xyz;
    MakeVertexHeightsAgree(p0, p1);
    const float t = SphereToScreenSpaceTessellation(p0, p1, diameter);
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
    if (patchIdx % 2 != 0) {
        p0 += (p0 - p1);
    } else { 
        p1 += (p1 - p0);
    }

    // Having moved the vertex in (x,z), its height is no longer correct.
    MakeVertexHeightsAgree(p0, p1);
    // Half the tessellation because the edge is twice as long.
    const float t = 0.5f * SphereToScreenSpaceTessellation(p0, p1, 2 * diameter);
    return LargerNeighbourAdjacencyClamp(t);
}

float getTessLevel(in int idx0, in int idx1, in float diameter) {
    const vec3 p0 = gl_in[idx0].gl_Position.xyz;
    const vec3 p1 = gl_in[idx1].gl_Position.xyz;

    const float tess = SphereToScreenSpaceTessellation(p0, p1, diameter);
#if 0
    return pow(2, ceil(log2(tess)));
#else
    return tess;
#endif
}

#define neighbourMinusX z
#define neighbourMinusY y
#define neighbourPlusX  x
#define neighbourPlusY  w

void main(void)
{
    const TerrainNodeData tData = dvd_TerrainData[dvd_instanceID[gl_InvocationID]];

    const vec3  centre = 0.25f * (gl_in[0].gl_Position.xyz + gl_in[1].gl_Position.xyz + gl_in[2].gl_Position.xyz + gl_in[3].gl_Position.xyz);
    const float sideLen = max(abs(gl_in[1].gl_Position.x - gl_in[0].gl_Position.x), abs(gl_in[1].gl_Position.x - gl_in[2].gl_Position.x));     // assume square & uniform
    const float diagLen = sqrt(2 * sideLen * sideLen);
    if (!inFrustum(centre, dvd_cameraPosition.xyz, dvd_cameraForward, diagLen))
    {
          gl_TessLevelInner[0] = gl_TessLevelInner[1] = -1;
          gl_TessLevelOuter[0] = gl_TessLevelOuter[1] = -1;
          gl_TessLevelOuter[2] = gl_TessLevelOuter[3] = -1;
    }
    else
    {
        PassData(gl_InvocationID);

        // Outer tessellation level
        gl_TessLevelOuter[0] = getTessLevel(0, 1, sideLen);
        gl_TessLevelOuter[1] = getTessLevel(3, 0, sideLen);
        gl_TessLevelOuter[2] = getTessLevel(2, 3, sideLen);
        gl_TessLevelOuter[3] = getTessLevel(1, 2, sideLen);

        // Edges that need adjacency adjustment are identified by the per-instance ip[0].adjacency 
        // scalars, in *conjunction* with a patch ID that puts them on the edge of a tile.
        const int PatchID = gl_PrimitiveID % 4;
        ivec2 patchXY;
        patchXY.y = PatchID / PATCHES_PER_TILE_EDGE;
        patchXY.x = PatchID - patchXY.y * PATCHES_PER_TILE_EDGE;
 
        const vec4 adj = tData._tScale;
        // Identify patch edges that are adjacent to a patch of a different size.  The size difference
        // is encoded in _in[n].adjacency, either 0.5, 1.0 or 2.0.
        // neighbourMinusX refers to our adjacent neighbour in the direction of -ve x.  The value 
        // is the neighbour's size relative to ours.  Similarly for plus and Y, etc.  You really
        // need a diagram to make sense of the adjacency conditions in the if statements. :-(
        if (patchXY.x == 0) {
            if (adj.neighbourMinusX < 0.55f) {
                // Deal with neighbours that are smaller.
                gl_TessLevelOuter[0] = SmallerNeighbourAdjacencyFix(0, 1, sideLen);
            } else if (adj.neighbourMinusX > 1.1f) {
                // Deal with neighbours that are larger than us. 
                gl_TessLevelOuter[0] = LargerNeighbourAdjacencyFix(0, 1, patchXY.y, sideLen);
            }
        } else if (patchXY.x == PATCHES_PER_TILE_EDGE - 1) {
            if (adj.neighbourPlusX < 0.55f) {
                gl_TessLevelOuter[2] = SmallerNeighbourAdjacencyFix(2, 3, sideLen);
            } else if (adj.neighbourPlusX > 1.1f) {
                gl_TessLevelOuter[2] = LargerNeighbourAdjacencyFix(3, 2, patchXY.y, sideLen);
            }
        }

        if (patchXY.y == 0) {
            if (adj.neighbourMinusY < 0.55f) {
                gl_TessLevelOuter[1] = SmallerNeighbourAdjacencyFix(3, 0, sideLen);
            } else if (adj.neighbourMinusY > 1.1f) {
                gl_TessLevelOuter[1] = LargerNeighbourAdjacencyFix(0, 3, patchXY.x, sideLen);	// NB: irregular index pattern - it's correct.
            }
        } else if (patchXY.y == PATCHES_PER_TILE_EDGE - 1) {
            if (adj.neighbourPlusY < 0.55f) {
                gl_TessLevelOuter[3] = SmallerNeighbourAdjacencyFix(1, 2, sideLen);
            } else if (adj.neighbourPlusY > 1.1f) {
                gl_TessLevelOuter[3] = LargerNeighbourAdjacencyFix(1, 2, patchXY.x, sideLen);	// NB: irregular index pattern - it's correct.
            }
        }
       // Inner tessellation level
       gl_TessLevelInner[0] = 0.5f * (gl_TessLevelOuter[0] + gl_TessLevelOuter[3]);
       gl_TessLevelInner[1] = 0.5f * (gl_TessLevelOuter[2] + gl_TessLevelOuter[1]);
    }

    // Pass the patch verts along
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    // Output tessellation level (used for wireframe coloring)
    tcs_tessLevel[gl_InvocationID] = gl_TessLevelOuter[0];
    tcs_tileScale[gl_InvocationID] = int(tData._positionAndTileScale.w);

}

--TessellationE

#if defined(SHADOW_PASS)
layout(quads, fractional_even_spacing, ccw) in;
#else
layout(quads, fractional_even_spacing, cw) in;
#endif

#include "nodeBufferedInput.cmn"

layout(binding = TEXTURE_HEIGHT) uniform sampler2D TexTerrainHeight;

layout(location = 10) in float tcs_tessLevel[];
layout(location = 11) in flat int tcs_tileScale[];

#if defined(TOGGLE_WIREFRAME) || defined(TOGGLE_NORMALS)
layout(location = 10) out int tes_tessLevel;
layout(location = 11) out int tes_tileScale;
layout(location = 12) out vec3 tes_tangentViewPos;
layout(location = 13) out vec3 tes_tangentFragPos;
#else
layout(location = 10) out flat int LoD;
// x = distance, y = depth
layout(location = 12) out vec3 _tangentViewPos;
layout(location = 13) out vec3 _tangentFragPos;
#endif

// Templates please!!!
vec2 Bilerp(vec2 v0, vec2 v1, vec2 v2, vec2 v3, vec2 i)
{
    float2 bottom = lerp(v0, v3, i.x);
    float2 top = lerp(v1, v2, i.x);
    float2 result = lerp(bottom, top, i.y);
    return result;
}

vec3 Bilerp(vec3 v0, vec3 v1, vec3 v2, vec3 v3, vec2 i)
{
    float3 bottom = lerp(v0, v3, i.x);
    float3 top = lerp(v1, v2, i.x);
    float3 result = lerp(bottom, top, i.y);
    return result;
}

vec4 getHeightOffsets(in vec2 tex_coord) {
    const ivec3 off = ivec3(-1, 0, 1);

    const float s01 = textureOffset(TexTerrainHeight, tex_coord, off.xy).r;
    const float s21 = textureOffset(TexTerrainHeight, tex_coord, off.zy).r;
    const float s10 = textureOffset(TexTerrainHeight, tex_coord, off.yx).r;
    const float s12 = textureOffset(TexTerrainHeight, tex_coord, off.yz).r;

    return (TERRAIN_HEIGHT_RANGE * vec4(s01, s21, s10, s12)) + vec4(TERRAIN_MIN_HEIGHT);
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

    // Calculate the vertex position using the four original points and interpolate depending on the tessellation coordinates.	
    vec3 pos = Bilerp(gl_in[0].gl_Position.xyz, gl_in[1].gl_Position.xyz, gl_in[2].gl_Position.xyz, gl_in[3].gl_Position.xyz, gl_TessCoord.xy);

    // Terrain heightmap coords
    _out._texCoord = Bilerp(_in[0]._texCoord, _in[1]._texCoord, _in[2]._texCoord, _in[3]._texCoord, gl_TessCoord.xy);

    // Sample the heightmap and offset y position of vertex
    const vec4 heightOffsets = getHeightOffsets(_out._texCoord);
    pos.y = getHeight(heightOffsets);

    _out._vertexW = vec4(pos, 1.0f);
    _out._vertexWV = dvd_ViewMatrix * _out._vertexW;
    _out._vertexWVP = dvd_ProjectionMatrix * _out._vertexWV;

#if !defined(SHADOW_PASS)
    const mat3 normalMatrix = mat3(dvd_Matrices[DATA_IDX]._normalMatrixW);

    const vec3 N = getNormal(pos.y, heightOffsets);
    const vec3 B = cross(vec3(0.0f, 0.0f, 1.0f), N);
    const vec3 T = cross(N, B);
    const mat3 TBN =  mat3(normalMatrix * T,
                           normalMatrix * B,
                           normalMatrix * N);
#if defined(PRE_PASS)
    _out._tbn = mat3(dvd_ViewMatrix) * TBN;
    _out._normalWV = _out._tbn[0];
#else
    _out._normalWV = normalize(mat3(dvd_ViewMatrix) * TBN[0]);
    _out._viewDirectionWV = normalize(-_out._vertexWV.xyz);
#endif

    const mat3 tTBN = transpose(TBN);
#if defined(TOGGLE_WIREFRAME) || defined(TOGGLE_NORMALS)
    tes_tangentViewPos = tTBN * dvd_cameraPosition.xyz;
    tes_tangentFragPos = tTBN * _out._vertexW.xyz;
#else
    _tangentViewPos = tTBN * dvd_cameraPosition.xyz;
    _tangentFragPos = tTBN * _out._vertexW.xyz;
#endif

#endif //SHADOW_PASS

#if defined(TOGGLE_WIREFRAME) || defined(TOGGLE_NORMALS)
    tes_tessLevel = int(tcs_tessLevel[0]);
    tes_tileScale = tcs_tileScale[0];
    gl_Position = _out._vertexW;
#else

    gl_Position = _out._vertexWVP;
    setClipPlanes();
#endif //TOGGLE_WIREFRAME

#if !defined(SHADOW_PASS)
#if !defined(TOGGLE_WIREFRAME) && !defined(TOGGLE_NORMALS)
    LoD = int(log2(max(tcs_tileScale[0], 16)) - 4);
#endif //!TOGGLE_WIREFRAME && !TOGGLE_NORMALS

#endif //SHADOW_PASS
}

--Geometry

#include "nodeBufferedInput.cmn"

layout(triangles) in;

layout(location = 10) in int tes_tessLevel[];
layout(location = 11) in int tes_tileScale[];
layout(location = 12) in vec3 tes_tangentViewPos[];
layout(location = 13) in vec3 tes_tangentFragPos[];

#define SHOW_TILE_SCALE

#if defined(TOGGLE_NORMALS)
layout(line_strip, max_vertices = 18) out;
#else
layout(triangle_strip, max_vertices = 4) out;
#endif

// x = distance, y = depth
layout(location = 10) out flat int LoD;

layout(location = 12) out vec3 gs_wireColor;
layout(location = 13) noperspective out vec3 gs_edgeDist;
layout(location = 14) out vec3 _tangentViewPos;
layout(location = 15) out vec3 _tangentFragPos;

vec4 getWVPPositon(in int i) {
    return dvd_ViewProjectionMatrix * gl_in[i].gl_Position;
}

void PerVertex(in int i, in vec3 edge_dist) {
    PassData(i);
    gl_Position = getWVPPositon(i);
    _tangentViewPos = tes_tangentViewPos[i];
    _tangentFragPos = tes_tangentFragPos[i];

#if !defined(SHADOW_PASS)
    LoD = int(log2(max(tes_tileScale[i], 16)) - 4);

#if !defined(TOGGLE_NORMALS)
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
#else //SHOW_TILE_SCALE
    if (tes_tileScale[i] == 256) {
        gs_wireColor = vec3(0.0, 0.0, 0.0);
    }
    else if (tes_tileScale[i] == 128) {
        gs_wireColor = vec3(1.0, 1.0, 0.0);
    }
    else if (tes_tileScale[i] == 64) {
        gs_wireColor = vec3(0.0, 0.0, 1.0);
    }
    else if (tes_tileScale[i] == 32) {
        gs_wireColor = vec3(0.0, 1.0, 0.0);
    }
    else if (tes_tileScale[i] == 16) {
        gs_wireColor = vec3(1.0, 0.0, 0.0);
    }
    else if (tes_tileScale[i] == 8) {
        gs_wireColor = vec3(1.0, 0.0, 1.0);
    } else {
        gs_wireColor = vec3(1.0, 1.0, 1.0);
    }
#endif //SHOW_TILE_SCALE
#endif //TOGGLE_NORMALS
    gs_edgeDist = vec3(i == 0 ? edge_dist.x : 0.0,
                       i == 1 ? edge_dist.y : 0.0,
                       i >= 2 ? edge_dist.z : 0.0);
#endif //SHADOW_PASS
    setClipPlanes();
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
    NodeData nodeData = dvd_Matrices[DATA_IDX];
    const float sizeFactor = 0.75f;
    for (int i = 0; i < count; ++i) {

        mat3 normalMatrixWV = mat3(dvd_ViewMatrix) * mat3(nodeData._normalMatrixW);
        mat3 normalMatrixWVInv = inverse(normalMatrixWV);
        mat3 tbn = normalMatrixWVInv * _in[i]._tbn;

        vec3 P = gl_in[i].gl_Position.xyz;
        vec3 T = tbn[0];
        vec3 B = tbn[1];
        vec3 N = tbn[2];

        gs_edgeDist = vec3(i == 0 ? edge_dist.x : 0.0,
                           i == 1 ? edge_dist.y : 0.0,
                           i >= 2 ? edge_dist.z : 0.0);

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
        EmitVertex();
    }

    // This closes the triangle
    PerVertex(0, edge_dist);
    EmitVertex();

    EndPrimitive();
#endif //TOGGLE_NORMALS
}

--Fragment.LQPass

#if !defined(PRE_PASS)
layout(early_fragment_tests) in;
#endif

layout(location = 10) in flat int LoD;
#if defined(TOGGLE_WIREFRAME) || defined(TOGGLE_NORMALS)
layout(location = 12) in vec3 gs_wireColor;
layout(location = 13) noperspective in vec3 gs_edgeDist;
layout(location = 14) in vec3 _tangentViewPos;
layout(location = 15) in vec3 _tangentFragPos;
#else
layout(location = 12) in vec3 _tangentViewPos;
layout(location = 13) in vec3 _tangentFragPos;
#endif

#define IS_TERRAIN
#define USE_CUSTOM_ROUGHNESS
#define SHADOW_INTENSITY_FACTOR 0.75f

#include "nodeBufferedInput.cmn"
#include "BRDF.frag"
#include "output.frag"
#include "waterData.cmn"

#if defined(MAX_TEXTURE_LAYERS)
#undef MAX_TEXTURE_LAYERS
#define MAX_TEXTURE_LAYERS 1
#endif

#include "terrainSplatting.frag"


#if !defined(PRE_PASS)
vec3 getOcclusionMetallicRoughness(in mat4 colourMatrix, in vec2 uv) {
    return vec3(0.0f, 0.0f, 0.8f);
}
#endif

void main(void)
{
    NodeData data = dvd_Matrices[DATA_IDX];
    prepareData(data);

    const vec2 waterData = waterDetails(VAR._vertexW.xyz, TERRAIN_MIN_HEIGHT);

#if defined(PRE_PASS)
    writeOutput(data, TexCoords, VAR._normalWV);
#else //PRE_PASS
#if !defined(TOGGLE_NORMALS)
    vec4 albedo;
    vec3 normalWV;
    BuildTerrainData(waterData, albedo, normalWV);

    vec4 colourOut = getPixelColour(vec4(albedo.rgb, 1.0f), data, normalWV, TexCoords);
#else//TOGGLE_NORMALS
    vec4 colourOut = vec4(gs_wireColor, 1.0f);
#endif//TOGGLE_NORMALS

#if defined(TOGGLE_WIREFRAME)
    const float LineWidth = 0.25f;
    const float d = min(min(gs_edgeDist.x, gs_edgeDist.y), gs_edgeDist.z);
    colourOut = mix(vec4(gs_wireColor, 1.0f), colourOut, smoothstep(LineWidth - 1, LineWidth + 1, d));
#endif//TOGGLE_WIREFRAME

    writeOutput(colourOut);
#endif //PRE_PASS
}

--Fragment.MainPass

#if !defined(PRE_PASS)
layout(early_fragment_tests) in;
#define IS_TERRAIN
#define USE_CUSTOM_ROUGHNESS
#define SHADOW_INTENSITY_FACTOR 0.75f
#else
#define USE_CUSTOM_NORMAL_MAP
#endif

#include "nodeBufferedInput.cmn"
#include "waterData.cmn"

layout(location = 10) in flat int LoD;
// x = distance, y = depth
#if defined(TOGGLE_WIREFRAME) || defined(TOGGLE_NORMALS)
layout(location = 12) in vec3 gs_wireColor;
layout(location = 13) noperspective in vec3 gs_edgeDist;
layout(location = 14) in vec3 _tangentViewPos;
layout(location = 15) in vec3 _tangentFragPos;
#else
layout(location = 12) in vec3 _tangentViewPos;
layout(location = 13) in vec3 _tangentFragPos;
#endif

#if defined(PRE_PASS)
#include "prePass.frag"
#else
#include "BRDF.frag"
#include "output.frag"
#endif

#include "terrainSplatting.frag"

#if !defined(PRE_PASS)
float _private_roughness = 0.0f;

vec3 getOcclusionMetallicRoughness(in mat4 colourMatrix, in vec2 uv) {
    return vec3(0.0f, 0.0f, _private_roughness);
}
#endif

void main(void)
{
    NodeData data = dvd_Matrices[DATA_IDX];
    prepareData(data);

    const vec2 waterData = waterDetails(VAR._vertexW.xyz, TERRAIN_MIN_HEIGHT);
#if defined(PRE_PASS)
    writeOutput(data, TexCoords, getMixedNormal(1.0f - waterData.x));
#else //PRE_PASS

#if !defined(TOGGLE_NORMALS)
    vec4 albedo;
    vec3 normalWV;
    BuildTerrainData(waterData, albedo, normalWV);

    _private_roughness = albedo.a;

    vec4 colourOut = getPixelColour(vec4(albedo.rgb, 1.0f), data, normalWV, TexCoords);
#else //TOGGLE_NORMALS
    vec4 colourOut = vec4(gs_wireColor, 1.0f);
#endif //TOGGLE_NORMALS

#if defined(TOGGLE_WIREFRAME)
    const float LineWidth = 0.75f;
    const float d = min(min(gs_edgeDist.x, gs_edgeDist.y), gs_edgeDist.z);
    colourOut = mix(vec4(gs_wireColor, 1.0f), colourOut, smoothstep(LineWidth - 1, LineWidth + 1, d));
#endif //TOGGLE_WIREFRAME

    writeOutput(colourOut);

#endif //PRE_PASS
}
