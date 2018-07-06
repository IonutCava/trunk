--Vertex

#include "vbInputData.vert"

uniform float TerrainLength;
uniform float TerrainWidth;
uniform vec3 TerrainOrigin;

struct TerrainNodeData {
    vec4 _positionAndTileScale;
    vec4 _tScale;
};

layout(binding = BUFFER_TERRAIN_DATA, std430) coherent readonly buffer dvd_TerrainBlock
{
    TerrainNodeData dvd_TerrainData[];
};

float tileScale = 0.0;

vec2 calcTerrainTexCoord(in vec4 pos)
{
    return vec2(abs(pos.x - TerrainOrigin.x) / TerrainWidth, abs(pos.z - TerrainOrigin.z) / TerrainLength);
}

void main(void)
{
    computeData();

    vec4 posAndScale = dvd_TerrainData[VAR.dvd_drawID]._positionAndTileScale;
    tileScale = posAndScale.w;
    vec4 offset = vec4(posAndScale.xyz, 0.0);

    vec4 patchPosition = vec4(dvd_Vertex.xyz * tileScale, 1.0);

    vec4 p = dvd_WorldMatrix(VAR.dvd_instanceID) * (patchPosition + offset);

    // Calcuate texture coordantes (u,v) relative to entire terrain
    VAR._texCoord = calcTerrainTexCoord(p);

    // Send vertex position along
    gl_Position = patchPosition;
}

--TessellationC

#include "nodeBufferedInput.cmn"

layout(binding = TEXTURE_OPACITY)   uniform sampler2D TexTerrainHeight;

uniform float TerrainHeightOffset = 2000.0f;

struct TerrainNodeData {
    vec4 _positionAndTileScale;
    vec4 _tScale;
};

layout(binding = BUFFER_TERRAIN_DATA, std430) coherent readonly buffer dvd_TerrainBlock
{
    TerrainNodeData dvd_TerrainData[];
};


float tscale_negx = 0.0;
float tscale_negz = 0.0;
float tscale_posx = 0.0;
float tscale_posz = 0.0;

//
// Outputs
//
layout(vertices = 4) out;

patch out float gl_TessLevelOuter[4];
patch out float gl_TessLevelInner[2];

out vec2 tcs_terrainTexCoord[];
out float tcs_tessLevel[];

/**
* Dynamic level of detail using camera distance algorithm.
*/
float dlodCameraDistance(vec4 p0, vec4 p1, vec2 t0, vec2 t1)
{
    vec4 samp = texture(TexTerrainHeight, t0);
    p0.y = samp[0] * TerrainHeightOffset;
    samp = texture(TexTerrainHeight, t1);
    p1.y = samp[0] * TerrainHeightOffset;

    mat4 mvMatrix = dvd_WorldViewMatrix(VAR.dvd_instanceID);

    vec3 offset = dvd_TerrainData[VAR.dvd_drawID]._positionAndTileScale.xyz;
    vec4 view0 = mvMatrix * vec4(p0.xyz + offset, p0.w);
    vec4 view1 = mvMatrix * vec4(p1.xyz + offset, p1.w);

    float MinDepth = 10.0;
    float MaxDepth = 100000.0;

    float d0 = clamp((abs(p0.z) - MinDepth) / (MaxDepth - MinDepth), 0.0, 1.0);
    float d1 = clamp((abs(p1.z) - MinDepth) / (MaxDepth - MinDepth), 0.0, 1.0);

    float t = mix(64, 2, (d0 + d1) * 0.5);

    if (t <= 2.0)
    {
        return 2.0;
    }
    if (t <= 4.0)
    {
        return 4.0;
    }
    if (t <= 8.0)
    {
        return 8.0;
    }
    if (t <= 16.0)
    {
        return 16.0;
    }
    if (t <= 32.0)
    {
        return 32.0;
    }

    return 64.0;
}

/**
* Dynamic level of detail using sphere algorithm.
* Source adapated from the DirectX 11 Terrain Tessellation example.
*/
float dlodSphere(vec4 p0, vec4 p1, vec2 t0, vec2 t1)
{
    float g_tessellatedTriWidth = 10.0;

    vec4 samp = texture(TexTerrainHeight, t0);
    p0.y = samp[0] * TerrainHeightOffset;
    samp = texture(TexTerrainHeight, t1);
    p1.y = samp[0] * TerrainHeightOffset;


    mat4 mvMatrix = dvd_WorldViewMatrix(VAR.dvd_instanceID);
    vec3 offset = dvd_TerrainData[VAR.dvd_drawID]._positionAndTileScale.xyz;

    vec4 center = 0.5 * (p0 + p1);
    vec4 view0 = mvMatrix * vec4(center.xyz + offset, center.w);
    vec4 view1 = view0;
    view1.x += distance(p0, p1);

    vec4 clip0 = dvd_ProjectionMatrix * view0;
    vec4 clip1 = dvd_ProjectionMatrix * view1;

    clip0 /= clip0.w;
    clip1 /= clip1.w;

    //clip0.xy *= dvd_ViewPort.zw;
    //clip1.xy *= dvd_ViewPort.zw;

    vec2 screen0 = ((clip0.xy + 1.0) / 2.0) * dvd_ViewPort.zw;
    vec2 screen1 = ((clip1.xy + 1.0) / 2.0) * dvd_ViewPort.zw;
    float d = distance(screen0, screen1);

    // g_tessellatedTriWidth is desired pixels per tri edge
    float t = clamp(d / g_tessellatedTriWidth, 0, 64);

    if (t <= 2.0)
    {
        return 2.0;
    }
    if (t <= 4.0)
    {
        return 4.0;
    }
    if (t <= 8.0)
    {
        return 8.0;
    }
    if (t <= 16.0)
    {
        return 16.0;
    }
    if (t <= 32.0)
    {
        return 32.0;
    }

    return 64.0;
}

void main(void)
{
    tscale_negx = dvd_TerrainData[VAR.dvd_drawID]._tScale.x;
    tscale_negz = dvd_TerrainData[VAR.dvd_drawID]._tScale.y;
    tscale_posx = dvd_TerrainData[VAR.dvd_drawID]._tScale.z;
    tscale_posz = dvd_TerrainData[VAR.dvd_drawID]._tScale.w;

    // Outer tessellation level
    gl_TessLevelOuter[0] = dlodCameraDistance(gl_in[3].gl_Position, gl_in[0].gl_Position, tcs_terrainTexCoord[3], tcs_terrainTexCoord[0]);
    gl_TessLevelOuter[1] = dlodCameraDistance(gl_in[0].gl_Position, gl_in[1].gl_Position, tcs_terrainTexCoord[0], tcs_terrainTexCoord[1]);
    gl_TessLevelOuter[2] = dlodCameraDistance(gl_in[1].gl_Position, gl_in[2].gl_Position, tcs_terrainTexCoord[1], tcs_terrainTexCoord[2]);
    gl_TessLevelOuter[3] = dlodCameraDistance(gl_in[2].gl_Position, gl_in[3].gl_Position, tcs_terrainTexCoord[2], tcs_terrainTexCoord[3]);

    if (tscale_negx == 2.0)
        gl_TessLevelOuter[0] = max(2.0, gl_TessLevelOuter[0] * 0.5);
    if (tscale_negz == 2.0)
        gl_TessLevelOuter[1] = max(2.0, gl_TessLevelOuter[1] * 0.5);
    if (tscale_posx == 2.0)
        gl_TessLevelOuter[2] = max(2.0, gl_TessLevelOuter[2] * 0.5);
    if (tscale_posz == 2.0)
        gl_TessLevelOuter[3] = max(2.0, gl_TessLevelOuter[3] * 0.5);

    // Inner tessellation level
    gl_TessLevelInner[0] = 0.5 * (gl_TessLevelOuter[0] + gl_TessLevelOuter[3]);
    gl_TessLevelInner[1] = 0.5 * (gl_TessLevelOuter[2] + gl_TessLevelOuter[1]);

    // Pass the patch verts along
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

    PassData(gl_InvocationID);

    // Output tessellation level (used for wireframe coloring)
    tcs_tessLevel[gl_InvocationID] = gl_TessLevelOuter[0];
}

--TessellationE

#include "nodeBufferedInput.cmn"

layout(binding = TEXTURE_OPACITY)   uniform sampler2D TexTerrainHeight;

struct TerrainNodeData {
    vec4 _positionAndTileScale;
    vec4 _tScale;
};

layout(binding = BUFFER_TERRAIN_DATA, std430) coherent readonly buffer dvd_TerrainBlock
{
    TerrainNodeData dvd_TerrainData[];
};


uniform float TerrainHeightOffset = 2000.0;

//
// Inputs
//
layout(quads, fractional_even_spacing) in;

patch in float gl_TessLevelOuter[4];
patch in float gl_TessLevelInner[2];

in float tcs_tessLevel[];

out float tes_tessLevel;

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

void main()
{
    // Calculate the vertex position using the four original points and interpolate depneding on the tessellation coordinates.	
    gl_Position = interpolate(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position, gl_in[3].gl_Position);

    
    // Terrain heightmap coords
    vec2 terrainTexCoord = interpolate2(VAR[0]._texCoord, VAR[1]._texCoord, VAR[2]._texCoord, VAR[3]._texCoord);

    // Sample the heightmap and offset y position of vertex
    vec4 samp = texture(TexTerrainHeight, terrainTexCoord);
    gl_Position.y = samp[0] * TerrainHeightOffset;

    // Project the vertex to clip space and send it along
    vec3 offset = dvd_TerrainData[VAR[0].dvd_drawID]._positionAndTileScale.xyz;
    gl_Position = dvd_ProjectionMatrix * dvd_WorldViewMatrix(VAR[0].dvd_instanceID) * vec4(gl_Position.xyz + offset, gl_Position.w);

    PassData(0);
    _out._texCoord = terrainTexCoord;
    tes_tessLevel = tcs_tessLevel[0];
}

--Geometry

uniform float ToggleWireframe = 1.0;

layout(triangles) in;

in float tes_tessLevel[];

layout(triangle_strip, max_vertices = 4) out;

out vec4 gs_wireColor;
noperspective out vec3 gs_edgeDist;

vec4 wireframeColor()
{
    if (tes_tessLevel[0] == 64.0)
        return vec4(0.0, 0.0, 1.0, 1.0);
    else if (tes_tessLevel[0] >= 32.0)
        return vec4(0.0, 1.0, 1.0, 1.0);
    else if (tes_tessLevel[0] >= 16.0)
        return vec4(1.0, 1.0, 0.0, 1.0);
    else if (tes_tessLevel[0] >= 8.0)
        return vec4(1.0, 1.0, 1.0, 1.0);
    else
        return vec4(1.0, 0.0, 0.0, 1.0);
}

void main(void)
{
    vec4 wireColor = wireframeColor();

    // Calculate edge distances for wireframe
    float ha, hb, hc;
    if (ToggleWireframe == 1.0)
    {
        vec2 p0 = vec2(dvd_ViewPort.zw * (gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w));
        vec2 p1 = vec2(dvd_ViewPort.zw * (gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w));
        vec2 p2 = vec2(dvd_ViewPort.zw * (gl_in[2].gl_Position.xy / gl_in[2].gl_Position.w));

        float a = length(p1 - p2);
        float b = length(p2 - p0);
        float c = length(p1 - p0);
        float alpha = acos((b*b + c*c - a*a) / (2.0*b*c));
        float beta = acos((a*a + c*c - b*b) / (2.0*a*c));
        ha = abs(c * sin(beta));
        hb = abs(c * sin(alpha));
        hc = abs(b * sin(alpha));
    }
    else
    {
        ha = hb = hc = 0.0;
    }

    // Output verts
    for (int i = 0; i < gl_in.length(); ++i)
    {
        PassData(i);

        gl_Position = gl_in[i].gl_Position;
        gs_wireColor = wireColor;

        if (i == 0)
            gs_edgeDist = vec3(ha, 0, 0);
        else if (i == 1)
            gs_edgeDist = vec3(0, hb, 0);
        else
            gs_edgeDist = vec3(0, 0, hc);

        EmitVertex();
    }

    // This closes the the triangle
    gl_Position = gl_in[0].gl_Position;
    gs_edgeDist = vec3(ha, 0, 0);
    _out._texCoord = VAR[0]._texCoord;

    gs_wireColor = wireColor;
    EmitVertex();

    EndPrimitive();
}

--Fragment

struct TerrainNodeData {
    vec4 _positionAndTileScale;
    vec4 _tScale;
};

layout(binding = BUFFER_TERRAIN_DATA, std430) coherent readonly buffer dvd_TerrainBlock
{
    TerrainNodeData dvd_TerrainData[];
};

float tileScale = dvd_TerrainData[VAR.dvd_drawID]._positionAndTileScale.w;

uniform float ToggleWireframe = 1.0;

layout(binding = TEXTURE_OPACITY)   uniform sampler2D TexTerrainHeight;

//
// Inputs
//
in vec4 gs_wireColor;
noperspective in vec3 gs_edgeDist;

//
// Ouputs
//
layout(location = 0) out vec4 _colourOut;
layout(location = 1) out vec2 _normalOut;
layout(location = 2) out vec2 _velocityOut;

void main(void)
{
    //vec4 color = vec4(mix(0.0, 1.0, tileScale / 1000.0), mix(1.0, 0.0, tileScale / 1000.0), 0.0, 1.0);
    vec4 color = vec4(1.0, 0.0, 0.0, 1.0);// texture(TexTerrainHeight, VAR._texCoord);

    // Wireframe junk
    float d = min(gs_edgeDist.x, gs_edgeDist.y);
    d = min(d, gs_edgeDist.z);

    float LineWidth = 0.75;
    float mixVal = smoothstep(LineWidth - 1, LineWidth + 1, d);

    if (ToggleWireframe == 1.0)
        _colourOut = mix(gs_wireColor, color, mixVal);
    else
        _colourOut = color;
}