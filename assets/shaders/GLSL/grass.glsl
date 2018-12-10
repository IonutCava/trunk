-- Vertex

#include "vbInputData.vert"

struct GrassData {
    mat4 transform;
    //x - width extent, y = height extent, z = array index, w - render
    vec4 data;
};

layout(std430, binding = BUFFER_GRASS_DATA) coherent readonly buffer dvd_transformBlock{
    GrassData grassData[];
};

flat out int _arrayLayerGS;
flat out int _cullFlag;

void main()
{
    computeDataMinimal();

    GrassData data = grassData[gl_InstanceID];
    _arrayLayerGS = int(data.data.z);
    _cullFlag = int(data.data.w);

    VAR._vertexW = data.transform * dvd_Vertex;
    VAR._vertexWV = dvd_ViewMatrix * VAR._vertexW;
    VAR._normalWV = mat3(dvd_ViewMatrix * data.transform) * dvd_Normal;

    //if (posOffset.y > 0.75) {
    //    computeFoliageMovementGrass(VAR._vertexW);
    //}

    //setClipPlanes(VAR._vertexW);

    //computeLightVectors();

    gl_Position = dvd_ProjectionMatrix * VAR._vertexWV;
}

--Geometry

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;


in flat int _arrayLayerGS[];
in flat int _cullFlag[];
out flat int _arrayLayerFrag;

void PerVertex(int idx) {
    PassData(idx);
    _arrayLayerFrag = _arrayLayerGS[idx];
    gl_Position = gl_in[idx].gl_Position;
#if !defined(SHADOW_PASS)
    setClipPlanes(gl_in[idx].gl_Position);
#endif
}

void main(void) {
    if (_cullFlag[0] == 1) 
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

//#include "BRDF.frag"
//#include "velocityCalc.frag"

#include "utility.frag"

layout(location = 0) out vec4 _colourOut;
layout(location = 1) out vec2 _normalOut;
layout(location = 2) out vec2 _velocityOut;

flat in int _arrayLayerFrag;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

void main (void){
    //vec4 colour = texture(texDiffuseGrass, vec3(VAR._texCoord, 0/*_arrayLayerFrag*/));
    /*if (colour.a < 1.0 - Z_TEST_SIGMA) {
        discard;
    }*/

    _colourOut = vec4(1.0, 0.0, 0.0, 1.0);// vec4(colour.rgb, 1.0);
    _normalOut = packNormal(vec3(1.0, 0.0, 0.0)/*normalize(VAR._normalWV)*/);
    _velocityOut = vec2(1.0, 1.0);//velocityCalc(dvd_InvProjectionMatrix, getScreenPositionNormalised());
}


--Fragment.PrePass

void main() {
}


--Fragment.Shadow

flat in int _arrayLayerFrag;

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
    if (colour.a < 1.0 - Z_TEST_SIGMA) discard;

    _colourOut = computeMoments(gl_FragCoord.z);
}