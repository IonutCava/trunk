--Vertex

layout(location = 0) out vec4 tScale;

#include "vbInputData.vert"

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

void main(void)
{
    computeDataNoClip();

    tScale = dvd_TerrainData[VAR.dvd_instanceID]._tScale;

    const vec4 posAndTileScaleVert = dvd_TerrainData[VAR.dvd_instanceID]._positionAndTileScale;
    // Send vertex position along
    gl_Position = vec4(dvd_Vertex.xyz * posAndTileScaleVert.w + posAndTileScaleVert.xyz, 1.0f);

    VAR._vertexWV = dvd_ViewMatrix * gl_Position;

    // Calculate texture coordinates (u,v) relative to entire terrain
    VAR._texCoord = calcTerrainTexCoord(gl_Position);
}

--TessellationC

#define id gl_InvocationID

layout(location = 0) in vec4 tScale[];

#include "nodeBufferedInput.cmn"

uniform vec2 tessellationRange;

//
// Outputs
//
layout(vertices = 4) out;

layout(location = 0) out float tcs_tessLevel[];

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

float saturate(in float val) { return clamp(val, 0.0f, 1.0f); }

uint nextPOW2(in uint v) {
    --v;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return ++v;
}

// Dynamic level of detail using camera distance algorithm.
float dlodCameraDistance(vec4 p0, vec4 p1, in vec2 t0, in vec2 t1, in mat4 viewMatrix)
{
#if MAX_TESS_SCALE != MIN_TESS_SCALE

    const vec4 view0 = viewMatrix * p0;
    const vec4 view1 = viewMatrix * p1;

    const float range = tessellationRange.y - tessellationRange.x;

    const float d0 = (abs(view0.z) - tessellationRange.x) / range;
    const float d1 = (abs(view1.z) - tessellationRange.x) / range;
    float mixVal = floor(mix(MAX_TESS_SCALE, MIN_TESS_SCALE, saturate((d0 + d1) * 0.5f)));
    uint pow2Val = nextPOW2(uint(mixVal));
    return clamp(pow2Val, MIN_TESS_SCALE, MAX_TESS_SCALE);
#else
     return MAX_TESS_SCALE;
#endif
}

void main(void)
{
    PassData(id);

    if (gl_InvocationID == 0 && 
        all(bvec4(offscreen(project(_in[0]._vertexWV, dvd_ProjectionMatrix)),
                  offscreen(project(_in[1]._vertexWV, dvd_ProjectionMatrix)),
                  offscreen(project(_in[2]._vertexWV, dvd_ProjectionMatrix)),
                  offscreen(project(_in[3]._vertexWV, dvd_ProjectionMatrix)))))
    {
        gl_TessLevelInner[0] = 0;
        gl_TessLevelInner[1] = 0;

        gl_TessLevelOuter[0] = 0;
        gl_TessLevelOuter[1] = 0;
        gl_TessLevelOuter[2] = 0;
        gl_TessLevelOuter[3] = 0;
    } else {
        // Outer tessellation level
        const float tessLevels[4] = {
            dlodCameraDistance(gl_in[3].gl_Position, gl_in[0].gl_Position, _in[3]._texCoord, _in[0]._texCoord, dvd_ViewMatrix),
            dlodCameraDistance(gl_in[0].gl_Position, gl_in[1].gl_Position, _in[0]._texCoord, _in[1]._texCoord, dvd_ViewMatrix),
            dlodCameraDistance(gl_in[1].gl_Position, gl_in[2].gl_Position, _in[1]._texCoord, _in[2]._texCoord, dvd_ViewMatrix),
            dlodCameraDistance(gl_in[2].gl_Position, gl_in[3].gl_Position, _in[2]._texCoord, _in[3]._texCoord, dvd_ViewMatrix)
        };

        for (int i = 0; i < 4; ++i) {
            gl_TessLevelOuter[i] = max(MIN_TESS_SCALE, tessLevels[i] * tScale[id][i]);
        }

        // Inner tessellation level
        gl_TessLevelInner[0] = (gl_TessLevelOuter[0] + gl_TessLevelOuter[3]) * 0.5f;
        gl_TessLevelInner[1] = (gl_TessLevelOuter[2] + gl_TessLevelOuter[1]) * 0.5f;
    }

    // Pass the patch verts along
    gl_out[id].gl_Position = gl_in[id].gl_Position;

    // Output tessellation level (used for wireframe coloring)
    tcs_tessLevel[id] = gl_TessLevelOuter[0];
}

--TessellationE

layout(quads, fractional_even_spacing) in;

#include "nodeBufferedInput.cmn"

layout(binding = TEXTURE_HEIGHT) uniform sampler2D TexTerrainHeight;

layout(location = 0) in float tcs_tessLevel[];

#if defined(TOGGLE_WIREFRAME) || defined(TOGGLE_NORMALS)
layout(location = 0) out float tes_tessLevel;
#else
layout(location = 0) out flat int LoD;
// x = distance, y = depth
layout(location = 1) smooth out vec2 _waterDetails;
#endif

vec4 interpolate(in vec4 v0, in vec4 v1, in vec4 v2, in vec4 v3)
{
    const vec4 a = mix(v0, v1, gl_TessCoord.x);
    const vec4 b = mix(v3, v2, gl_TessCoord.x);
    return mix(a, b, gl_TessCoord.y);
}

vec2 interpolate2(in vec2 v0, in vec2 v1, in vec2 v2, in vec2 v3)
{
    const vec2 a = mix(v0, v1, gl_TessCoord.x);
    const vec2 b = mix(v3, v2, gl_TessCoord.x);
    return mix(a, b, gl_TessCoord.y);
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
#if 0
    const vec2 size = vec2(2.0f, 0.0f);


    const float s11 = sampleHeight;
    const float s01 = heightOffsets.x;
    const float s21 = heightOffsets.y;
    const float s10 = heightOffsets.z;
    const float s12 = heightOffsets.w;

    //vec3 va = normalize(vec3(size.xy, s21 - s01));
    //vec3 vb = normalize(vec3(size.yx, s12 - s10));

    vec3 va = normalize(vec3(size.x, s21 - s01, size.y));
    vec3 vb = normalize(vec3(size.y, s12 - s10, -size.x));
    return normalize(cross(va, vb) * 0.5f + 0.5f);
#else
    float hL = heightOffsets.x;
    float hR = heightOffsets.y;
    float hD = heightOffsets.z;
    float hU = heightOffsets.w;

    // deduce terrain normal
    return normalize(vec3(hL - hR, 2.0f, hD - hU));
#endif
}

#if !defined(TOGGLE_WIREFRAME) && !defined(TOGGLE_NORMALS)
void waterDetails() {

    vec3 vertexW = _out._vertexW.xyz;

    float maxDistance = 0.0f;
    float minDepth = 1.0f;

    const int entityCount = dvd_waterEntities.length();

    for (int i = 0; i < entityCount; ++i)
    {
        WaterBodyData data = dvd_waterEntities[i];

        vec4 details = data.details;
        vec4 position = data.positionW;

        float waterWidth = details.x + position.x;
        float waterLength = details.y + position.z;
        float halfWidth = waterWidth * 0.5f;
        float halfLength = waterLength * 0.5f;
        if (vertexW.x >= -halfWidth && vertexW.x <= halfWidth &&
            vertexW.z >= -halfLength && vertexW.z <= halfLength)
        {
            float depth = -details.y + position.y;

            // Distance
            maxDistance = max(maxDistance, 1.0 - smoothstep(position.y - 0.05f, position.y + 0.05f, vertexW.y));


            // Current water depth in relation to the minimum possible depth
            minDepth = min(minDepth, clamp(1.0 - (position.y - vertexW.y) / (position.y - TERRAIN_MIN_HEIGHT), 0.0f, 1.0f));
        }
    }
#if !defined(SHADOW_PASS)
    _waterDetails = vec2(maxDistance, minDepth);
#endif
}

#endif

void main()
{
    PassData(0);

    const uint baseInstance = VAR[0].dvd_baseInstance;

    // Terrain heightmap coords
    _out._texCoord = interpolate2(VAR[0]._texCoord, VAR[1]._texCoord, VAR[2]._texCoord, VAR[3]._texCoord);
    const vec4 heightOffsets = getHeightOffsets(_out._texCoord);

    // Calculate the vertex position using the four original points and interpolate depending on the tessellation coordinates.	
    vec4 pos = interpolate(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position, gl_in[3].gl_Position);
    // Sample the heightmap and offset y position of vertex
    pos.y = getHeight(heightOffsets);

    mat4 worldMat = dvd_WorldMatrix(baseInstance);
    // Project the vertex to clip space and send it along
    _out._vertexW = worldMat * pos;

#if !defined(SHADOW_PASS)
    mat3 normalMatrixW = dvd_NormalMatrixW(baseInstance);
    mat3 normalMatrixWV = mat3(dvd_ViewMatrix) * normalMatrixW;

    const vec3 N = getNormal(pos.y, heightOffsets);
    const vec3 B = cross(vec3(0.0f, 0.0f, 1.0f), N);
    const vec3 T = cross(N, B);

    _out._normalW = normalMatrixW * N;
    _out._normalWV = normalize(normalMatrixWV * N);
    _out._tbn = normalMatrixWV * mat3(T, B, N);

#endif
    _out._vertexWV = dvd_ViewMatrix * _out._vertexW;

#if defined(TOGGLE_WIREFRAME) || defined(TOGGLE_NORMALS)
    tes_tessLevel = tcs_tessLevel[0];
    gl_Position = _out._vertexW;
#else
    gl_Position = dvd_ProjectionMatrix * _out._vertexWV;
    setClipPlanes(_out._vertexW);

#if !defined(SHADOW_PASS)
    waterDetails();
#if !defined(TOGGLE_WIREFRAME) || defined(TOGGLE_NORMALS)
    LoD = int(log2(MAX_TESS_SCALE / tcs_tessLevel[0]));
#endif //TOGGLE_WIREFRAME
#endif //SHADOW_PASS

#endif
}

--Geometry

#include "nodeBufferedInput.cmn"

layout(triangles) in;

layout(location = 0) in float tes_tessLevel[];

#if defined(TOGGLE_NORMALS)
layout(line_strip, max_vertices = 18) out;
#else
layout(triangle_strip, max_vertices = 4) out;
#endif

// x = distance, y = depth
layout(location = 0) out flat int LoD;
layout(location = 1) smooth out vec2 _waterDetails;

layout(location = 2) out vec3 gs_wireColor;
layout(location = 3) noperspective out vec3 gs_edgeDist;


#if !defined(SHADOW_PASS)
void waterDetails(in int index) {

    vec3 vertexW = VAR[index]._vertexW.xyz;

    float maxDistance = 0.0f;
    float minDepth = 1.0;

    for (int i = 0; i < dvd_waterEntities.length(); ++i)
    {
        WaterBodyData data = dvd_waterEntities[i];
        
        vec4 details = data.details;
        vec4 position = data.positionW;

        float waterWidth = details.x + position.x;
        float waterLength = details.y + position.z;
        float halfWidth = waterWidth * 0.5;
        float halfLength = waterLength * 0.5;
        if (vertexW.x >= -halfWidth && vertexW.x <= halfWidth &&
            vertexW.z >= -halfLength && vertexW.z <= halfLength)
        {
            float depth = -details.y + position.y;

            // Distance
            maxDistance = max(maxDistance, 1.0 - smoothstep(position.y - 0.05f, position.y + 0.05f, vertexW.y));


            // Current water depth in relation to the minimum possible depth
            minDepth = min(minDepth, clamp(1.0 - (position.y - vertexW.y) / (position.y - TERRAIN_MIN_HEIGHT), 0.0, 1.0));
        }
    }
        
    _waterDetails = vec2(maxDistance, minDepth);
}
#endif

vec4 getWVPPositon(int index) {
    return dvd_ViewProjectionMatrix * gl_in[index].gl_Position;
}

void PerVertex(in int i, in vec3 edge_dist) {
    PassData(i);
    gl_Position = getWVPPositon(i);
#if !defined(SHADOW_PASS)
    LoD = int(log2(MAX_TESS_SCALE / tes_tessLevel[0]));

    if (tes_tessLevel[0] >= 64.0) {
        gs_wireColor = vec3(0.0, 0.0, 1.0);
    } else if (tes_tessLevel[0] >= 32.0) {
        gs_wireColor = vec3(0.0, 1.0, 0.0);
    } else if (tes_tessLevel[0] >= 16.0) {
        gs_wireColor = vec3(1.0, 0.0, 0.0);
    } else if (tes_tessLevel[0] >= 8.0) {
        gs_wireColor = vec3(0.25, 1.00, 1.0);
    } else {
        gs_wireColor = vec3(1.0, 1.0, 1.0);
    }
    waterDetails(i);


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

#if defined(TOGGLE_NORMALS)

    const float sizeFactor = 0.75f;
    const int count = gl_in.length();
    for (int i = 0; i < count; ++i) {

        mat3 normalMatrixWVInv = inverse(dvd_NormalMatrixWV(_in[i].dvd_baseInstance));
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

layout(location = 0) in flat int LoD;
layout(location = 1) smooth in vec2 _waterDetails;

#define NO_SPECULAR

#if defined(TOGGLE_WIREFRAME) || defined(TOGGLE_NORMALS)
layout(location = 2) in vec3 gs_wireColor;
layout(location = 3) noperspective in vec3 gs_edgeDist;
#endif

#define NEED_DEPTH_TEXTURE
#include "BRDF.frag"
#include "output.frag"

#if defined(MAX_TEXTURE_LAYERS)
#undef MAX_TEXTURE_LAYERS
#define MAX_TEXTURE_LAYERS 1
#endif

#include "terrainSplatting.frag"

vec4 UnderwaterAlbedo(in vec2 uv) {
    return texture(helperTextures, vec3(uv * UNDERWATER_TILE_SCALE, 1));
}

void main(void)
{
    vec2 uv = getTexCoord();
    const float crtDepth = computeDepth(VAR._vertexWV);
    const vec3 normal = TerrainNormal(uv, crtDepth);

    vec4 albedo = mix(getTerrainAlbedo(uv), UnderwaterAlbedo(uv), _waterDetails.x);
    mat4 colourMatrix = dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix;
    vec4 colourOut = getPixelColour(albedo, colourMatrix, normalize(normal), uv);

#if defined(TOGGLE_NORMALS)
    colourOut = vec4(gs_WireColor, 1.0f);
#elif defined(TOGGLE_WIREFRAME)
    const float LineWidth = 0.75f;
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

layout(location = 0) in flat int LoD;
// x = distance, y = depth
layout(location = 1) smooth in vec2 _waterDetails;

#define SHADOW_INTENSITY_FACTOR 0.75f

#if defined(TOGGLE_WIREFRAME) || defined(TOGGLE_NORMALS)
layout(location = 2) in vec3 gs_wireColor;
layout(location = 3) noperspective in vec3 gs_edgeDist;
#endif

#if defined(PRE_PASS)
#include "prePass.frag"
#else
#define NEED_DEPTH_TEXTURE
#include "BRDF.frag"
#include "output.frag"
#endif

#include "terrainSplatting.frag"

#if !defined(PRE_PASS)

vec4 UnderwaterAlbedo(in vec2 uv) {
    vec2 coords = uv * UNDERWATER_TILE_SCALE;
    float time2 = float(dvd_time) * 0.0001f;
    vec4 scrollingUV = vec4(coords, coords + time2);
    scrollingUV.s -= time2;

    return mix((texture(helperTextures, vec3(scrollingUV.st, 0)) + texture(helperTextures, vec3(scrollingUV.pq, 0))) * 0.5f,
                texture(helperTextures, vec3(coords, 1)),
                _waterDetails.y);
}

#else

vec3 UnderwaterNormal(in vec2 uv) {
    return (VAR._tbn * normalize(2.0f * texture(helperTextures, vec3(uv * UNDERWATER_TILE_SCALE, 2)).rgb - 1.0f));
}
#endif

void main(void)
{
    vec2 uv = getTexCoord();

#if defined(PRE_PASS)
    const float crtDepth = computeDepth(VAR._vertexWV);
    const vec3 normal = normalize(mix(TerrainNormal(uv, crtDepth), UnderwaterNormal(uv), _waterDetails.x));
    outputWithVelocity(uv, 1.0f, crtDepth, normal);
#else
    vec4 albedo = mix(getTerrainAlbedo(uv), UnderwaterAlbedo(uv), _waterDetails.x);
    mat4 colourMatrix = dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix;
    vec4 colourOut = getPixelColour(albedo, colourMatrix, getNormal(uv), uv);

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
