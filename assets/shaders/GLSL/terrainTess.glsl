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

    // Calculate texture coordinates (u,v) relative to entire terrain
    VAR._texCoord = calcTerrainTexCoord(gl_Position);
}

--TessellationC

#define id gl_InvocationID

layout(location = 0) in vec4 tScale[];

#include "nodeBufferedInput.cmn"
//layout(binding = TEXTURE_HEIGHT) uniform sampler2D TexTerrainHeight;

uniform vec2 tessellationRange;

//
// Outputs
//
layout(vertices = 4) out;

layout(location = 0) out float tcs_tessLevel[];

const vec2 cmp = vec2(1.7f);

bool offscreen(in vec4 vertex) {
    return vertex.z < -0.5f ||
           any(lessThan(vertex.xy, -cmp)) ||
           any(greaterThan(vertex.xy, cmp));
}

vec4 project(in vec4 vertex, in mat4 mvp) {
    const vec4 result = mvp * vertex;
    return result / result.w;
}

uint nextPOW2(in uint n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;

    return ++n;
}

// Dynamic level of detail using camera distance algorithm.
float dlodCameraDistance(vec4 p0, vec4 p1, in vec2 t0, in vec2 t1, in mat4 viewMatrix)
{
    if (MAX_TESS_SCALE != MIN_TESS_SCALE) {
        //p0.y = (TERRAIN_HEIGHT_RANGE * texture(TexTerrainHeight, t0).r) + TERRAIN_MIN_HEIGHT;
        //p1.y = (TERRAIN_HEIGHT_RANGE * texture(TexTerrainHeight, t1).r) + TERRAIN_MIN_HEIGHT;

        const vec4 view0 = viewMatrix * p0;
        const vec4 view1 = viewMatrix * p1;

        const float d0 = clamp((abs(view0.z) - tessellationRange.x) / (tessellationRange.y - tessellationRange.x), 0.0f, 1.0f);
        const float d1 = clamp((abs(view1.z) - tessellationRange.x) / (tessellationRange.y - tessellationRange.x), 0.0f, 1.0f);
#if 1
        return mix(MAX_TESS_SCALE, MIN_TESS_SCALE, (d0 + d1) * 0.5f);
#else

        const uint t = nextPOW2(uint(mix(MAX_TESS_SCALE, MIN_TESS_SCALE, (d0 + d1) * 0.5f)));

        return float(clamp(t, uint(MIN_TESS_SCALE), uint(MAX_TESS_SCALE)));
#endif
    }

    return MAX_TESS_SCALE;
}

void main(void)
{
    PassData(id);

    const mat4 mvp = dvd_ViewProjectionMatrix * dvd_WorldMatrix(VAR.dvd_baseInstance);

    const vec4 v0 = project(gl_in[0].gl_Position, mvp);
    const vec4 v1 = project(gl_in[1].gl_Position, mvp);
    const vec4 v2 = project(gl_in[2].gl_Position, mvp);
    const vec4 v3 = project(gl_in[3].gl_Position, mvp);

    if (gl_InvocationID == 0 && all(bvec4(offscreen(v0), offscreen(v1), offscreen(v2), offscreen(v3)))) {
        gl_TessLevelInner[0] = 0;
        gl_TessLevelInner[1] = 0;

        gl_TessLevelOuter[0] = 0;
        gl_TessLevelOuter[1] = 0;
        gl_TessLevelOuter[2] = 0;
        gl_TessLevelOuter[3] = 0;
    } else {
        // Outer tessellation level
        gl_TessLevelOuter[0] = dlodCameraDistance(gl_in[3].gl_Position, gl_in[0].gl_Position, _in[3]._texCoord, _in[0]._texCoord, dvd_ViewMatrix);
        gl_TessLevelOuter[1] = dlodCameraDistance(gl_in[0].gl_Position, gl_in[1].gl_Position, _in[0]._texCoord, _in[1]._texCoord, dvd_ViewMatrix);
        gl_TessLevelOuter[2] = dlodCameraDistance(gl_in[1].gl_Position, gl_in[2].gl_Position, _in[1]._texCoord, _in[2]._texCoord, dvd_ViewMatrix);
        gl_TessLevelOuter[3] = dlodCameraDistance(gl_in[2].gl_Position, gl_in[3].gl_Position, _in[2]._texCoord, _in[3]._texCoord, dvd_ViewMatrix);

        gl_TessLevelOuter[0] = max(2.0f, gl_TessLevelOuter[0] * tScale[id].x);
        gl_TessLevelOuter[1] = max(2.0f, gl_TessLevelOuter[1] * tScale[id].y);
        gl_TessLevelOuter[2] = max(2.0f, gl_TessLevelOuter[2] * tScale[id].z);
        gl_TessLevelOuter[3] = max(2.0f, gl_TessLevelOuter[3] * tScale[id].w);

        // Inner tessellation level
        gl_TessLevelInner[0] = 0.5f * (gl_TessLevelOuter[0] + gl_TessLevelOuter[3]);
        gl_TessLevelInner[1] = 0.5f * (gl_TessLevelOuter[2] + gl_TessLevelOuter[1]);
    }

    // Pass the patch verts along
    gl_out[id].gl_Position = gl_in[id].gl_Position;

    // Output tessellation level (used for wireframe coloring)
    tcs_tessLevel[id] = gl_TessLevelOuter[0];
}

--TessellationE

#include "nodeBufferedInput.cmn"

layout(binding = TEXTURE_HEIGHT) uniform sampler2D TexTerrainHeight;

layout(quads, fractional_even_spacing) in;

layout(location = 0) in float tcs_tessLevel[];

#if defined(TOGGLE_WIREFRAME)
layout(location = 0) out float tes_tessLevel;
#else
layout(location = 0) out flat int LoD;
// x = distance, y = depth
layout(location = 1) smooth out vec2 _waterDetails;
#endif

vec4 interpolate(in vec4 v0, in vec4 v1, in vec4 v2, in vec4 v3)
{
    vec4 a = mix(v0, v1, gl_TessCoord.x);
    vec4 b = mix(v3, v2, gl_TessCoord.x);
    return mix(a, b, gl_TessCoord.y);
}

vec2 interpolate2(in vec2 v0, in vec2 v1, in vec2 v2, in vec2 v3)
{
    vec2 a = mix(v0, v1, gl_TessCoord.x);
    vec2 b = mix(v3, v2, gl_TessCoord.x);
    return mix(a, b, gl_TessCoord.y);
}

vec4 getHeightOffsets(in vec2 tex_coord) {
    const ivec3 off = ivec3(-1, 0, 1);

    float s01 = textureOffset(TexTerrainHeight, tex_coord, off.xy).r;
    float s21 = textureOffset(TexTerrainHeight, tex_coord, off.zy).r;
    float s10 = textureOffset(TexTerrainHeight, tex_coord, off.yx).r;
    float s12 = textureOffset(TexTerrainHeight, tex_coord, off.yz).r;

    return (TERRAIN_HEIGHT_RANGE * vec4(s01, s21, s10, s12)) + TERRAIN_MIN_HEIGHT;
}

float getHeight(in vec4 heightOffsets) {
    
    float s01 = heightOffsets.r;
    float s21 = heightOffsets.g;
    float s10 = heightOffsets.b;
    float s12 = heightOffsets.a;

    return (s01 + s21 + s10 + s12) * 0.25f;
}

vec3 getNormal(in float sampleHeight, in vec4 heightOffsets) {
    float s01 = heightOffsets.r;
    float s21 = heightOffsets.g;
    float s10 = heightOffsets.b;
    float s12 = heightOffsets.a;

    return normalize(vec3(s21 - s01, 2.0f, s12 - s10));
}

#if !defined(TOGGLE_WIREFRAME)
void waterDetails() {

    vec3 vertexW = _out._vertexW.xyz;

    float maxDistance = 0.0f;
    float minDepth = 1.0f;

    for (int i = 0; i < dvd_waterEntities.length(); ++i)
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

    // Calculate the vertex position using the four original points and interpolate depending on the tessellation coordinates.	
    vec4 pos = interpolate(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position, gl_in[3].gl_Position);

    // Terrain heightmap coords
    _out._texCoord = interpolate2(VAR[0]._texCoord, VAR[1]._texCoord, VAR[2]._texCoord, VAR[3]._texCoord);

    const vec4 heightOffsets = getHeightOffsets(_out._texCoord);

    // Sample the heightmap and offset y position of vertex
    pos.y = getHeight(heightOffsets);

    // Project the vertex to clip space and send it along
    _out._vertexW = dvd_WorldMatrix(baseInstance) * pos;

#if !defined(SHADOW_PASS)
    mat3 normalMatrixWV = dvd_NormalMatrixWV(baseInstance);
    vec3 normal = getNormal(pos.y, heightOffsets);

    _out._normalWV = normalize(normalMatrixWV * normal);

    const vec3 bitangent = cross(vec3(0.0f, 0.0f, 1.0f), normal);
    const vec3 tangnet = cross(normal, bitangent);

    _out._tbn = mat3(
                    normalize(normalMatrixWV * tangnet),
                    normalize(normalMatrixWV * bitangent),
                    _out._normalWV
                );
#endif
    _out._vertexWV = dvd_ViewMatrix * _out._vertexW;

#if defined(TOGGLE_WIREFRAME)
    tes_tessLevel = tcs_tessLevel[0];
    gl_Position = _out._vertexW;
#else
    gl_Position = dvd_ViewProjectionMatrix * _out._vertexW;
    setClipPlanes(_out._vertexW);

#if !defined(SHADOW_PASS)
    waterDetails();
#if !defined(TOGGLE_WIREFRAME)
    LoD = int(log2(MAX_TESS_SCALE / tcs_tessLevel[0]));
#endif //TOGGLE_WIREFRAME
#endif //SHADOW_PASS

#endif
}

--Geometry

#include "nodeBufferedInput.cmn"

layout(triangles) in;

layout(location = 0) in float tes_tessLevel[];

layout(triangle_strip, max_vertices = 4) out;

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
        float alpha = acos((b*b + c*c - a*a) / (2.0*b*c));
        float beta = acos((a*a + c*c - b*b) / (2.0*a*c));
        edge_dist.x = abs(c * sin(beta));
        edge_dist.y = abs(c * sin(alpha));
        edge_dist.z = abs(b * sin(alpha));
    }

    // Output verts
    for (int i = 0; i < gl_in.length(); ++i) {
        PerVertex(i, edge_dist);
        EmitVertex();
    }

    // This closes the triangle
    PerVertex(0, edge_dist);
    EmitVertex();

    EndPrimitive();
}

--Fragment

#if !defined(PRE_PASS)
layout(early_fragment_tests) in;
#else
#define USE_CUSTOM_NORMAL_MAP
#endif

layout(location = 0) in flat int LoD;
// x = distance, y = depth
layout(location = 1) smooth in vec2 _waterDetails;

#define SHADOW_INTENSITY_FACTOR 0.75f

#if defined(TOGGLE_WIREFRAME)
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

layout(binding = TEXTURE_UNIT1)  uniform sampler2DArray underwaterTextures;

#if !defined(PRE_PASS)
// ToDo: Move this above the includes
#if defined(LOW_QUALITY)
#if defined(MAX_TEXTURE_LAYERS)
#undef MAX_TEXTURE_LAYERS
#define MAX_TEXTURE_LAYERS 1
#endif
#endif

vec4 UnderwaterAlbedo(in vec2 uv) {
    vec2 coords = uv * UNDERWATER_TILE_SCALE;
#if defined(LOW_QUALITY)
    return texture(underwaterTextures, vec3(coords, 1));
#else
    float time2 = float(dvd_time) * 0.0001f;
    vec4 scrollingUV = vec4(coords, coords + time2);
    scrollingUV.s -= time2;

    return mix((texture(underwaterTextures, vec3(scrollingUV.st, 0)) + texture(underwaterTextures, vec3(scrollingUV.pq, 0))) * 0.5f,
                texture(underwaterTextures, vec3(coords, 1)),
                _waterDetails.y);
#endif
}

#else

#if !defined(LOW_QUALITY)
vec3 UnderwaterNormal(in vec2 uv) {
    return normalize(2.0f * texture(underwaterTextures, vec3(uv * UNDERWATER_TILE_SCALE, 2)).rgb - 1.0f);
}
#endif //LQ

#endif

void main(void)
{
    vec2 uv = getTexCoord();

#if defined(PRE_PASS)
#if defined(LOW_QUALITY)
    const vec3 normal = TerrainNormal(uv);
#else
    const vec3 normal = mix(TerrainNormal(uv), UnderwaterNormal(uv), _waterDetails.x);
#endif

    outputWithVelocity(uv, normalize(VAR._tbn * normal));
#else

    vec4 albedo = mix(getTerrainAlbedo(uv), UnderwaterAlbedo(uv), _waterDetails.x);
    mat4 colourMatrix = dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix;
    vec4 colourOut = getPixelColour(albedo, colourMatrix, getNormal(uv), uv);

#if defined(TOGGLE_WIREFRAME)
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
