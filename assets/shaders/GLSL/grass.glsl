-- Vertex
#include "foliage.vert"
#include "vbInputData.vert"
#include "lightingDefaults.vert"

struct GrassData {
    mat4 transform;
    vec4 positionAndIndex;
    vec4 extentAndRender;
};

layout(std430, binding = BUFFER_GRASS_DATA) coherent readonly buffer dvd_transformBlock{
    GrassData grassData[];
};

flat out int _arrayLayer;

void main()
{
    computeDataMinimal();

    GrassData data = grassData[gl_InstanceID];
    _arrayLayer = int(data.positionAndIndex.w);

    VAR._vertexW = data.transform * dvd_Vertex;
    VAR._normalWV = mat3(dvd_ViewMatrix * data.transform) * dvd_Normal;

    //if (posOffset.y > 0.75) {
    //    computeFoliageMovementGrass(VAR._vertexW);
    //}

    //setClipPlanes(VAR._vertexW);

    //computeLightVectors();

    gl_Position = dvd_ViewProjectionMatrix * VAR._vertexW;
}

-- Fragment.Colour

#include "BRDF.frag"
#include "velocityCalc.frag"

flat in int _arrayLayer;
layout(location = 0) out vec4 _colourOut;
layout(location = 1) out vec2 _normalOut;
layout(location = 2) out vec2 _velocityOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

void main (void){
    vec4 colour = texture(texDiffuseGrass, vec3(VAR._texCoord, _arrayLayer));
    /*if (colour.a < 1.0 - Z_TEST_SIGMA) {
        discard;
    }*/

    _colourOut = colour;
    _normalOut = packNormal(normalize(VAR._normalWV));
    _velocityOut = velocityCalc(dvd_InvProjectionMatrix, getScreenPositionNormalised());
}

--Fragment.Shadow

flat in int _arrayLayer;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

out vec2 _colourOut;

vec2 computeMoments(in float depth) {
    // Compute partial derivatives of depth.  
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    // Compute second moment over the pixel extents.  
    return vec2(depth, depth*depth + 0.25*(dx*dx + dy*dy));
}

void main(void){
    vec4 colour = texture(texDiffuseGrass, vec3(VAR._texCoord, _arrayLayer));
    if (colour.a < 1.0 - Z_TEST_SIGMA) discard;

    _colourOut = computeMoments(gl_FragCoord.z);
}

--Fragment.PrePass

layout(early_fragment_tests) in;

flat in int _arrayLayer;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

void main(void){
    vec4 colour = texture(texDiffuseGrass, vec3(VAR._texCoord, _arrayLayer));
    if (colour.a < 1.0 - Z_TEST_SIGMA) discard;

}