-- Vertex

#include "vbInputData.vert"

out vec4 _scrollingUV;

void main(void) {

    computeData();

    float time2 = float(dvd_time) * 0.0001;
    vec2 noiseUV = VAR._texCoord * UNDERWATER_DIFFUSE_SCALE;
    _scrollingUV.st = noiseUV;
    _scrollingUV.pq = noiseUV + time2;
    _scrollingUV.s -= time2;

    gl_Position = dvd_ViewProjectionMatrix * VAR._vertexW;
}

--Fragment.Colour

layout(early_fragment_tests) in;

#include "utility.frag"
#include "velocityCalc.frag"

in vec4 _scrollingUV;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texWaterCaustics;

layout(location = 0) out vec4 _colourOut;
layout(location = 1) out vec2 _normalOut;
layout(location = 2) out vec2 _velocityOut;

void main(void) {
    _colourOut = (texture(texWaterCaustics, _scrollingUV.st) +
                  texture(texWaterCaustics, _scrollingUV.pq)) * 0.5;

    _normalOut = packNormal(normalize(VAR._normalWV));
    _velocityOut = velocityCalc(dvd_InvProjectionMatrix, getScreenPositionNormalised());
}

--Fragment.PrePass

void main() {

}