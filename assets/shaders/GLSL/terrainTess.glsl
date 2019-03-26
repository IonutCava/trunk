--Vertex

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
    vec2 TerrainOrigin = vec2(-(TERRAIN_WIDTH * 0.5), -(TERRAIN_LENGTH * 0.5));
    return vec2(abs(pos.x - TerrainOrigin.x) / TERRAIN_WIDTH, abs(pos.z - TerrainOrigin.y) / TERRAIN_LENGTH);
}

void main(void)
{
    computeDataNoClip();

    vec4 posAndScale = dvd_TerrainData[VAR.dvd_instanceID]._positionAndTileScale;

    vec4 patchPosition = vec4(dvd_Vertex.xyz * posAndScale.w, 1.0);
    
    VAR._vertexW = vec4(patchPosition + vec4(posAndScale.xyz, 0.0));

    // Calculate texture coordinates (u,v) relative to entire terrain
    VAR._texCoord = calcTerrainTexCoord(VAR._vertexW);

    // Send vertex position along
    gl_Position = patchPosition;
}

--TessellationC

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

#define tscale_negx tScale.x
#define tscale_negz tScale.y
#define tscale_posx tScale.z
#define tscale_posz tScale.w

uniform vec2 tessellationRange;

//
// Outputs
//
layout(vertices = 4) out;

out float tcs_tessLevel[];

bool offscreen(vec4 vertex) {
    if (vertex.z < -0.5) {
        return true;
    }

    return any(lessThan(vertex.xy, vec2(-1.7))) ||
           any(greaterThan(vertex.xy, vec2(1.7)));
}

vec4 project(mat4 mvp, vec4 vertex) {
    vec4 result = mvp * vertex;
    result /= result.w;
    return result;
}

vec2 screen_space(vec4 vertex) {
    return (clamp(vertex.xy, -1.3, 1.3) + 1) * (dvd_ViewPort.zw*0.5);
}

// Dynamic level of detail using camera distance algorithm.
float dlodCameraDistance(vec4 p0, vec4 p1)
{
    float d0 = clamp((abs(p0.z) - tessellationRange.x) / (tessellationRange.y - tessellationRange.x), 0.0, 1.0);
    float d1 = clamp((abs(p1.z) - tessellationRange.x) / (tessellationRange.y - tessellationRange.x), 0.0, 1.0);

    float t = mix(MAX_TESS_SCALE, MIN_TESS_SCALE, (d0 + d1) * 0.5);

    if (t <= 2.0) {
        return 2.0;
    }

    if (t <= 4.0) {
        return 4.0;
    }

    if (t <= 8.0) {
        return 8.0;
    }

    if (t <= 16.0) {
        return 16.0;
    }

    if (t <= 32.0){
        return 32.0;
    }

    return MAX_TESS_SCALE;
}

void main(void)
{
    PassData(id);

    mat4 mvp = dvd_ProjectionMatrix * dvd_ViewMatrix * dvd_WorldMatrix(VAR.dvd_baseInstance);

    vec4 v0 = project(mvp, gl_in[0].gl_Position);
    vec4 v1 = project(mvp, gl_in[1].gl_Position);
    vec4 v2 = project(mvp, gl_in[2].gl_Position);
    vec4 v3 = project(mvp, gl_in[3].gl_Position);

    if (gl_InvocationID == 0 && all(bvec4(offscreen(v0), offscreen(v1), offscreen(v2), offscreen(v3)))) {
        gl_TessLevelInner[0] = 0;
        gl_TessLevelInner[1] = 0;
        gl_TessLevelOuter[0] = 0;
        gl_TessLevelOuter[1] = 0;
        gl_TessLevelOuter[2] = 0;
        gl_TessLevelOuter[3] = 0;
    } else {
        // Outer tessellation level
        gl_TessLevelOuter[0] = dlodCameraDistance(gl_in[3].gl_Position, gl_in[0].gl_Position);
        gl_TessLevelOuter[1] = dlodCameraDistance(gl_in[0].gl_Position, gl_in[1].gl_Position);
        gl_TessLevelOuter[2] = dlodCameraDistance(gl_in[1].gl_Position, gl_in[2].gl_Position);
        gl_TessLevelOuter[3] = dlodCameraDistance(gl_in[2].gl_Position, gl_in[3].gl_Position);

        vec4 tScale = dvd_TerrainData[VAR.dvd_instanceID]._tScale;

        if (tscale_negx == 2.0) {
            gl_TessLevelOuter[0] = max(2.0, gl_TessLevelOuter[0] * 0.5);
        }

        if (tscale_negz == 2.0) {
            gl_TessLevelOuter[1] = max(2.0, gl_TessLevelOuter[1] * 0.5);
        }

        if (tscale_posx == 2.0) {
            gl_TessLevelOuter[2] = max(2.0, gl_TessLevelOuter[2] * 0.5);
        }

        if (tscale_posz == 2.0) {
            gl_TessLevelOuter[3] = max(2.0, gl_TessLevelOuter[3] * 0.5);
        }

        // Inner tessellation level
        gl_TessLevelInner[0] = 0.5 * (gl_TessLevelOuter[0] + gl_TessLevelOuter[3]);
        gl_TessLevelInner[1] = 0.5 * (gl_TessLevelOuter[2] + gl_TessLevelOuter[1]);
    }

    // Pass the patch verts along
    gl_out[id].gl_Position = gl_in[id].gl_Position;

    // Output tessellation level (used for wireframe coloring)
    tcs_tessLevel[id] = gl_TessLevelOuter[0];
}

--TessellationE

#include "nodeBufferedInput.cmn"

layout(binding = TEXTURE_HEIGHT) uniform sampler2D TexTerrainHeight;

struct TerrainNodeData {
    vec4 _positionAndTileScale;
    vec4 _tScale;
};

layout(binding = BUFFER_TERRAIN_DATA, std140) uniform dvd_TerrainBlock
{
    TerrainNodeData dvd_TerrainData[MAX_RENDER_NODES];
};

layout(quads, fractional_even_spacing) in;

in float tcs_tessLevel[];
out float tes_tessLevel;

#if !defined(TOGGLE_WIREFRAME)
// x = distance, y = depth
smooth out vec2 _waterDetails;
out float LoD;
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

    return vec4(s01, s21, s10, s12);
}

float getHeight(in vec4 heightOffsets) {
    
    float s01 = heightOffsets.r;
    float s21 = heightOffsets.g;
    float s10 = heightOffsets.b;
    float s12 = heightOffsets.a;

    return (s01 + s21 + s10 + s12) * 0.25;
}

vec3 getNormal(in float sampleHeight, in vec4 heightOffsets) {
    const vec2 size = vec2(2.0, 0.0);

    float s11 = sampleHeight;
    float s01 = heightOffsets.r;
    float s21 = heightOffsets.g;
    float s10 = heightOffsets.b;
    float s12 = heightOffsets.a;

    vec3 va = normalize(vec3(size.xy, s21 - s01));
    vec3 vb = normalize(vec3(size.yx, s12 - s10));

    return cross(va, vb);
}

vec3 getTangent(in vec3 normal) {
    vec3 temp_tangent = vec3(0.0, 0.0, 1.0);
    vec3 bitangent = cross(temp_tangent, normal);
    vec3 tangent = cross(normal, bitangent);
    return tangent;
}

#if !defined(TOGGLE_WIREFRAME)
void waterDetails() {

    vec3 vertexW = _out._vertexW.xyz;

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
#if !defined(SHADOW_PASS)
    _waterDetails = vec2(maxDistance, minDepth);
#endif
}

#endif

void main()
{
    PassData(0);

    // Calculate the vertex position using the four original points and interpolate depending on the tessellation coordinates.	
    vec4 pos = interpolate(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position, gl_in[3].gl_Position);

    // Terrain heightmap coords
    _out._texCoord = interpolate2(VAR[0]._texCoord, VAR[1]._texCoord, VAR[2]._texCoord, VAR[3]._texCoord);

    vec4 heightOffsets = getHeightOffsets(_out._texCoord);

    // Sample the heightmap and offset y position of vertex
    float sampleHeight = getHeight(heightOffsets);
    pos.y = (TERRAIN_HEIGHT_RANGE * sampleHeight) + TERRAIN_MIN_HEIGHT;

    // Project the vertex to clip space and send it along
    vec3 offset = dvd_TerrainData[VAR[0].dvd_instanceID]._positionAndTileScale.xyz;
    _out._vertexW = dvd_WorldMatrix(VAR[0].dvd_baseInstance) * vec4(pos.xyz + offset, pos.w);

#if !defined(SHADOW_PASS)
    mat3 normalMatrixWV = dvd_NormalMatrixWV(VAR[0].dvd_baseInstance);
    vec3 normal = getNormal(sampleHeight, heightOffsets);

    _out._normalWV = normalize(normalMatrixWV * normal);
    _out._tangentWV = normalize(normalMatrixWV * getTangent(normal));
    _out._bitangentWV = normalize(cross(_out._normalWV, _out._tangentWV));
#endif
    _out._vertexWV = dvd_ViewMatrix * _out._vertexW;

    tes_tessLevel = tcs_tessLevel[0];
#if defined(TOGGLE_WIREFRAME)
    gl_Position = _out._vertexW;

#else
    gl_Position = dvd_ViewProjectionMatrix * _out._vertexW;
    setClipPlanes(_out._vertexW);

#if !defined(SHADOW_PASS)
    waterDetails();
    if (tes_tessLevel >= 64.0) {
        LoD = 0;
    } else if (tes_tessLevel >= 32.0) {
        LoD = 1;
    } else if (tes_tessLevel >= 16.0) {
        LoD = 2;
    } else if (tes_tessLevel >= 8.0) {
        LoD = 3;
    } else {
        LoD = 4;
    }
#endif //SHADOW_PASS

#endif
}

--Geometry.Wireframe

#include "nodeBufferedInput.cmn"

layout(triangles) in;

in float tes_tessLevel[];

layout(triangle_strip, max_vertices = 4) out;

// x = distance, y = depth
smooth out vec2 _waterDetails;
out vec3 gs_wireColor;
out float LoD;
noperspective out vec3 gs_edgeDist;
#endif


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
    if (tes_tessLevel[0] >= 64.0) {
        gs_wireColor = vec3(0.0, 0.0, 1.0);
        LoD = 0;
    } else if (tes_tessLevel[0] >= 32.0) {
        gs_wireColor = vec3(0.0, 1.0, 0.0);
        LoD = 1;
    } else if (tes_tessLevel[0] >= 16.0) {
        gs_wireColor = vec3(1.0, 0.0, 0.0);
        LoD = 2;
    } else if (tes_tessLevel[0] >= 8.0) {
        gs_wireColor = vec3(0.25, 0.50, 0.75);
        LoD = 3;
    } else {
        gs_wireColor = vec3(1.0, 1.0, 1.0);
        LoD = 4;
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

layout(early_fragment_tests) in;

in float LoD;
// x = distance, y = depth
smooth in vec2 _waterDetails;

#if defined(TOGGLE_WIREFRAME)
in vec3 gs_wireColor;
noperspective in vec3 gs_edgeDist;
#endif

#include "BRDF.frag"
#include "terrainSplatting.frag"
#include "output.frag"

// ToDo: Move this above the includes
#if defined(LOW_QUALITY)
#if defined(MAX_TEXTURE_LAYERS)
#undef MAX_TEXTURE_LAYERS
#define MAX_TEXTURE_LAYERS 1
#endif
#endif

vec4 UnderwaterMappingRoutine(out vec3 normalWV) {
    vec2 coords = dvd_TexCoord * UNDERWATER_TILE_SCALE;
    vec3 tbn = normalize(2.0 * texture(texUnderwaterDetail, coords).rgb - 1.0);
    normalWV = normalize(getTBNMatrix() * tbn);

#if defined(LOW_QUALITY)
    return texture(texUnderwaterAlbedo, coords);
#else
    float time2 = float(dvd_time) * 0.0001;
    vec4 scrollingUV = vec4(coords, coords + time2);
    scrollingUV.s -= time2;

    return mix(texture(texWaterCaustics, scrollingUV.st) + texture(texWaterCaustics, scrollingUV.pq) * 0.5,
               texture(texUnderwaterAlbedo, coords),
               _waterDetails.y);
#endif
}

vec4 TerrainMappingRoutine(out vec3 normalWV) {
    vec4 albedo;
#if defined(LOW_QUALITY)
    albedo = getTerrainAlbedo();
    normalWV = VAR._normalWV;
#else
    vec3 normalTBN = getTerrainAlbedoAndNormalTBN(albedo);
    normalWV = normalize(getTBNMatrix() * normalTBN);
#endif

    return albedo;
}

void main(void)
{
    updateTexCoord();

    vec3 normalWV = vec3(0.0);
    vec4 albedo = mix(TerrainMappingRoutine(normalWV), UnderwaterMappingRoutine(normalWV), _waterDetails.x);

    mat4 colourMatrix = dvd_Matrices[VAR.dvd_baseInstance]._colourMatrix;
    vec4 colourOut = getPixelColour(albedo, colourMatrix, normalWV);

#if defined(TOGGLE_WIREFRAME)
    const float LineWidth = 0.75;
    float d = min(min(gs_edgeDist.x, gs_edgeDist.y), gs_edgeDist.z);
    colourOut = mix(vec4(gs_wireColor, 1.0), colourOut, smoothstep(LineWidth - 1, LineWidth + 1, d));
#endif

    writeOutput(colourOut, packNormal(normalWV));
}

--Fragment.Shadow

out vec2 _colourOut;

#include "nodeBufferedInput.cmn"
#include "vsm.frag"

void main() {

    _colourOut = computeMoments();
}
