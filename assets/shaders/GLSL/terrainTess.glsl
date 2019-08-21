--Vertex

#include "vbInputData.vert"

layout(binding = TEXTURE_HEIGHT) uniform sampler2D TexTerrainHeight;

struct TerrainNodeData {
    vec4 _positionAndTileScale;
    vec4 _tScale;
};

layout(binding = BUFFER_TERRAIN_DATA, std140) uniform dvd_TerrainBlock
{
    TerrainNodeData dvd_TerrainData[MAX_RENDER_NODES];
};

vec2 calcTerrainTexCoord(in vec4 pos)
{
    vec2 TerrainOrigin = vec2(-(TERRAIN_WIDTH * 0.5f), -(TERRAIN_LENGTH * 0.5f));
    return vec2(abs(pos.x - TerrainOrigin.x) / TERRAIN_WIDTH, abs(pos.z - TerrainOrigin.y) / TERRAIN_LENGTH);
}

float getHeight(in vec2 tex_coord) {
    return (TERRAIN_HEIGHT_RANGE * texture(TexTerrainHeight, tex_coord).r) + TERRAIN_MIN_HEIGHT;
}

void main(void)
{
    _out.dvd_baseInstance = DATA_IDX;
    _out.dvd_instanceID = gl_InstanceID;
    _out.dvd_drawID = gl_DrawIDARB;

    float iv = floor(gl_VertexID * (1.0f / CONTROL_VTX_PER_TILE_EDGE));
    float iu = gl_VertexID - iv * CONTROL_VTX_PER_TILE_EDGE;
    float u = (2 * (iu / (CONTROL_VTX_PER_TILE_EDGE - 1.0f))) - 1.0f;
    float v = (2 * (iv / (CONTROL_VTX_PER_TILE_EDGE - 1.0f))) - 1.0f;
    
    const vec4 posAndTileScaleVert = dvd_TerrainData[_out.dvd_instanceID]._positionAndTileScale;

    dvd_Vertex = vec4(vec3(u, 0.0f, v) * posAndTileScaleVert.w + posAndTileScaleVert.xyz, 1.0f);

    _out._vertexW = dvd_WorldMatrix(DATA_IDX) * dvd_Vertex;

    // Calculate texture coordinates (u,v) relative to entire terrain
    _out._texCoord = calcTerrainTexCoord(_out._vertexW);
    _out._vertexW.y = getHeight(_out._texCoord);
    _out._vertexWV = dvd_ViewMatrix * _out._vertexW;
    // Send vertex position along
    gl_Position = _out._vertexW;
}

--TessellationC

// Most of the stuff here is from nVidia's DX11 terrain tessellation sample

#define id gl_InvocationID

#include "nodeBufferedInput.cmn"

struct TerrainNodeData {
    vec4 _positionAndTileScale;
    vec4 _tScale;
};

layout(binding = BUFFER_TERRAIN_DATA, std140) uniform dvd_TerrainBlock
{
    TerrainNodeData dvd_TerrainData[MAX_RENDER_NODES];
};


//
// Outputs
//
layout(vertices = 4) out;
layout(location = 10) out float tcs_tessLevel[];

layout(binding = TEXTURE_HEIGHT) uniform sampler2D TexTerrainHeight;
uniform float tessTriangleWidth = 30.0f;
uniform vec2 tessellationRange;

float saturate(in float val) { return clamp(val, 0.0f, 1.0f); }

bool offscreen(in vec4 vertex) {
    const vec2 cmp = vec2(1.7f);

    return vertex.z < -0.5f ||
           any(lessThan(vertex.xy, -cmp)) ||
           any(greaterThan(vertex.xy, cmp));
}

vec4 project(in vec4 vertexWV, in mat4 proj) {
    const vec4 result = proj * vertexWV;
    return result / result.w;
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
    return clamp(d / tessTriangleWidth, MIN_TESS_SCALE, MAX_TESS_SCALE);
}

float SphereToScreenSpaceTessellation(vec3 p0, vec3 p1, in float diameter)
{
#if MAX_TESS_SCALE != MIN_TESS_SCALE
    vec3 center = 0.5f * (p0 + p1);
    vec4 view0 = dvd_ViewMatrix * vec4(center, 1.0f);
    vec4 view1 = view0;
    view1.x += diameter;

    vec4 clip0 = dvd_ProjectionMatrix * view0;
    vec4 clip1 = dvd_ProjectionMatrix * view1;
    return ClipToScreenSpaceTessellation(clip0, clip1);
#else
    return MAX_TESS_SCALE;
#endif
}

// Dynamic level of detail using camera distance algorithm.
float dlodCameraDistance(vec3 p0, vec3 p1, in float diameter)
{
#if MAX_TESS_SCALE != MIN_TESS_SCALE
    const vec4 view0 = dvd_ViewMatrix * vec4(p0, 1.0f);
    const vec4 view1 = dvd_ViewMatrix * vec4(p1, 1.0f);
    const float MinDepth = max(tessellationRange.x, 0.0001f);
    const float MaxDepth = max(MinDepth + 1.0f, tessellationRange.y);

    const float d0 = saturate((abs(view0.z) - MinDepth) / (MaxDepth - MinDepth));
    const float d1 = saturate((abs(view1.z) - MinDepth) / (MaxDepth - MinDepth));
    const float t = floor(mix(MAX_TESS_SCALE, MIN_TESS_SCALE, (d0 + d1) * 0.5f));
    float logTess = ceil(log2(t));
    return min(pow(2, logTess), uint(MAX_TESS_SCALE));
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

void MakeVertexHeightsAgree(inout vec3 p0, inout vec3 p1, in vec2 t0, in vec2 t1)
{
    p0.y = getHeight(t0);
    p1.y = getHeight(t1);
}

float SmallerNeighbourAdjacencyFix(in int idx0, in int idx1, in float diameter) {
    vec3 p0 = gl_in[idx0].gl_Position.xyz;
    vec3 p1 = gl_in[idx1].gl_Position.xyz;
    vec2 t0 = _in[idx0]._texCoord;
    vec2 t1 = _in[idx1]._texCoord;

    MakeVertexHeightsAgree(p0, p1, t0, t1);
    float t = SphereToScreenSpaceTessellation(p0, p1, diameter);
    return SmallerNeighbourAdjacencyClamp(t);
}


float LargerNeighbourAdjacencyFix(in int idx0, in int idx1, in int patchIdx, in float diameter) {
    vec3 p0 = gl_in[idx0].gl_Position.xyz;
    vec3 p1 = gl_in[idx1].gl_Position.xyz;
    vec2 t0 = _in[idx0]._texCoord;
    vec2 t1 = _in[idx1]._texCoord;

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
    MakeVertexHeightsAgree(p0, p1, t0, t1);


    // Half the tessellation because the edge is twice as long.
    float t = 0.5f * SphereToScreenSpaceTessellation(p0, p1, 2 * diameter);
    return LargerNeighbourAdjacencyClamp(t);
}

float getTessLevel(in int idx0, in int idx1, in float diameter) {
    vec3 p0 = gl_in[idx0].gl_Position.xyz;
    vec3 p1 = gl_in[idx1].gl_Position.xyz;
    vec2 t0 = _in[idx0]._texCoord;
    vec2 t1 = _in[idx1]._texCoord;

#if 0
    return dlodCameraDistance(p0, p1, diameter);
#else
    return SphereToScreenSpaceTessellation(p0, p1, diameter);
#endif
}

void main(void)
{
    if (gl_InvocationID == 0 && 
        all(bvec4(offscreen(project(_in[0]._vertexWV, dvd_ProjectionMatrix)),
                  offscreen(project(_in[1]._vertexWV, dvd_ProjectionMatrix)),
                  offscreen(project(_in[2]._vertexWV, dvd_ProjectionMatrix)),
                  offscreen(project(_in[3]._vertexWV, dvd_ProjectionMatrix)))))
    {
        gl_TessLevelInner[0] = gl_TessLevelInner[1] = -1;
        gl_TessLevelOuter[0] = gl_TessLevelOuter[1] = -1;
        gl_TessLevelOuter[2] = gl_TessLevelOuter[3] = -1;
    } 
    else
    {
        PassData(id);

        // square and uniform patches
        const float sideLen = max(abs(gl_in[1].gl_Position.x - gl_in[0].gl_Position.x), abs(gl_in[1].gl_Position.x - gl_in[2].gl_Position.x));
        const vec4 scaleFactor = dvd_TerrainData[VAR.dvd_instanceID]._tScale;

#       define posz 3
#       define posx 0
#       define negz 1
#       define negx 2

#       define tscale_negx scaleFactor[negx]
#       define tscale_posx scaleFactor[posx]
#       define tscale_negz scaleFactor[negz]
#       define tscale_posz scaleFactor[posz]

        // Outer tessellation level
        gl_TessLevelOuter[0] = getTessLevel(0, 1, sideLen);
        gl_TessLevelOuter[3] = getTessLevel(1, 2, sideLen);
        gl_TessLevelOuter[2] = getTessLevel(2, 3, sideLen);
        gl_TessLevelOuter[1] = getTessLevel(3, 0, sideLen);

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
        if (tscale_negx < 0.55f && patchXY.x == 0)
            gl_TessLevelOuter[0] = SmallerNeighbourAdjacencyFix(0, 1, sideLen);
        if (tscale_negz < 0.55f && patchXY.y == 0)
            gl_TessLevelOuter[1] = SmallerNeighbourAdjacencyFix(3, 0, sideLen);
        if (tscale_posx < 0.55f && patchXY.x == PATCHES_PER_TILE_EDGE - 1)
            gl_TessLevelOuter[2] = SmallerNeighbourAdjacencyFix(2, 3, sideLen);
        if (tscale_posz < 0.55f && patchXY.y == PATCHES_PER_TILE_EDGE - 1)
            gl_TessLevelOuter[3] = SmallerNeighbourAdjacencyFix(1, 2, sideLen);

        // Deal with neighbours that are larger than us. 
        if (tscale_negx > 1 && patchXY.x == 0)
            gl_TessLevelOuter[0] = LargerNeighbourAdjacencyFix(0, 1, patchXY.y, sideLen);
        if (tscale_negz > 1 && patchXY.y == 0)
            gl_TessLevelOuter[1] = LargerNeighbourAdjacencyFix(0, 3, patchXY.x, sideLen);	// NB: irregular index pattern - it's correct.
        if (tscale_posx > 1 && patchXY.x == PATCHES_PER_TILE_EDGE - 1)
            gl_TessLevelOuter[2] = LargerNeighbourAdjacencyFix(3, 2, patchXY.y, sideLen);
        if (tscale_posz > 1 && patchXY.y == PATCHES_PER_TILE_EDGE - 1)
            gl_TessLevelOuter[3] = LargerNeighbourAdjacencyFix(1, 2, patchXY.x, sideLen);	// NB: irregular index pattern - it's correct.

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

layout(quads, fractional_even_spacing, cw) in;

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

    // Sample the heightmap and offset y position of vertex
	const vec4 heightOffsets = getHeightOffsets(_out._texCoord);
    pos.y = getHeight(heightOffsets);

    _out._vertexW = vec4(pos, 1.0f);
	_out._vertexWV = dvd_ViewMatrix * _out._vertexW;

#if !defined(SHADOW_PASS)
    const mat3 normalMatrixWV = dvd_NormalMatrixWV(DATA_IDX);

    const vec3 N = getNormal(pos.y, heightOffsets);
    _out._normalWV = normalize(normalMatrixWV * N);

#if defined(PRE_PASS)
    const vec3 B = cross(vec3(0.0f, 0.0f, 1.0f), N);
    const vec3 T = cross(N, B);

    _out._tbn = normalMatrixWV * mat3(T, B, N);
#endif //PRE_PASS
#endif //SHADOW_PASS

#if defined(TOGGLE_WIREFRAME) || defined(TOGGLE_NORMALS)
    tes_tessLevel = int(tcs_tessLevel[0]);
    gl_Position = _out._vertexW;
#else
    gl_Position = dvd_ProjectionMatrix * _out._vertexWV;
    setClipPlanes(_out._vertexW);
#endif //TOGGLE_WIREFRAME

#if !defined(SHADOW_PASS)
#if !defined(TOGGLE_WIREFRAME) && !defined(TOGGLE_NORMALS)
    _waterDetails = waterDetails(_out._vertexW.xyz, TERRAIN_MIN_HEIGHT);
#endif //TOGGLE_WIREFRAME

#if !defined(TOGGLE_WIREFRAME) || defined(TOGGLE_NORMALS)
    LoD = 0;// int(log2(MAX_TESS_SCALE / tcs_tessLevel[0]));
#endif //TOGGLE_WIREFRAME

#endif //SHADOW_PASS
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
    LoD = 0;// int(log2(MAX_TESS_SCALE / tes_tessLevel[0]));

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
    _waterDetails = waterDetails(_in[i]._vertexW.xyz, TERRAIN_MIN_HEIGHT);

    gs_edgeDist = vec3(i == 0 ? edge_dist.x : 0.0,
                       i == 1 ? edge_dist.y : 0.0,
                       i >= 2 ? edge_dist.z : 0.0);
#endif
    setClipPlanes(gl_in[i].gl_Position);
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
// x = distance, y = depth
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
layout(location = 12) in vec3 gs_wireColor;
layout(location = 13) noperspective in vec3 gs_edgeDist;
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
    const float crtDepth = computeDepth(_in._vertexWV);
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
