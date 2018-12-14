-- Vertex

#include "vbInputData.vert"
#include "foliage.vert"

struct GrassData {
    mat4 transform;
    //x - width extent, y = height extent, z = array index, w - render
    vec4 data;
};

layout(std430, binding = BUFFER_GRASS_DATA) coherent readonly buffer dvd_transformBlock{
    GrassData grassData[];
};

flat out int _arrayLayerGS;
flat out float _lod;

void main()
{
    computeDataMinimal();

    GrassData data = grassData[gl_InstanceID];
    _arrayLayerGS = int(data.data.z);
    _lod = data.data.w;

    VAR._vertexW = data.transform * dvd_Vertex;
    VAR._vertexWV = dvd_ViewMatrix * VAR._vertexW;
    VAR._normalWV = mat3(dvd_ViewMatrix * data.transform) * dvd_Normal;

    if (dvd_Vertex.y > 0.75) {
        computeFoliageMovementGrass(VAR._vertexW, data.data.y);
    }

    //computeLightVectors();

    gl_Position = dvd_ProjectionMatrix * VAR._vertexWV;
}

--Geometry

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;


in flat int _arrayLayerGS[];
in flat float _lod[];
out flat int _arrayLayerFrag;
out flat float _alphaFactor;

void PerVertex(int idx) {
    PassData(idx);
    _arrayLayerFrag = _arrayLayerGS[idx];
    _alphaFactor = min(_lod[idx], 1.0f);
    gl_Position = gl_in[idx].gl_Position;
#if !defined(SHADOW_PASS)
    setClipPlanes(gl_in[idx].gl_Position);
#endif
}

void main(void) {
    if (_lod[0] <= 2)
    {
        // Output verts
        for (int i = 0; i < 3; ++i) {
            PerVertex(i);
            EmitVertex();
        }

        EndPrimitive();
    }
}

-- Fragment.Colour

layout(early_fragment_tests) in;

#include "BRDF.frag"
#include "velocityCalc.frag"

#include "utility.frag"
#include "output.frag"

flat in int _arrayLayerFrag;
flat in float _alphaFactor;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

void main (void){
    vec4 colour = texture(texDiffuseGrass, vec3(VAR._texCoord, _arrayLayerFrag));
    colour.a *= (1.0 - _alphaFactor);

    writeOutput(colour,  packNormal(normalize(VAR._normalWV)));
}

--Fragment.PrePass

flat in int _arrayLayerFrag;
flat in float _alphaFactor;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

void main() {
    vec4 colour = texture(texDiffuseGrass, vec3(VAR._texCoord, _arrayLayerFrag));
    if (colour.a *  (1.0 - _alphaFactor) < 1.0 - Z_TEST_SIGMA) discard;
}


--Fragment.Shadow

flat in int _arrayLayerFrag;
flat in float _alphaFactor;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

out vec2 _colourOut;

vec2 computeMoments(in float depth) {
    // Compute partial derivatives of depth.  
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    // Compute second moment over the pixel extents.  
    return vec2(depth, depth*depth + 0.25*(dx*dx + dy * dy));
}

void main(void) {
    vec4 colour = texture(texDiffuseGrass, vec3(VAR._texCoord, _arrayLayerFrag));
    if (colour.a *  (1.0 - _alphaFactor) < 1.0 - Z_TEST_SIGMA) discard;

    _colourOut = computeMoments(gl_FragCoord.z);
}