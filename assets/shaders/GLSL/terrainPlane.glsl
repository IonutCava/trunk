-- Vertex

#include "vbInputData.vert"

out vec4 _scrollingUV;

void main(void) {

    computeData();

    float time2 = float(dvd_time) * 0.0001;
    vec2 noiseUV = VAR._texCoord * UNDERWATER_TILE_SCALE;
    _scrollingUV.st = noiseUV;
    _scrollingUV.pq = noiseUV + time2;
    _scrollingUV.s -= time2;

    gl_Position = dvd_ViewProjectionMatrix * VAR._vertexW;
}

--Fragment.Colour

layout(early_fragment_tests) in;

#include "utility.frag"
#include "output.frag"

in vec4 _scrollingUV;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texWaterCaustics;


void main(void) {
    vec4 colourOut = (texture(texWaterCaustics, _scrollingUV.st) +
                      texture(texWaterCaustics, _scrollingUV.pq)) * 0.5;

    writeOutput(colourOut);
}

--Fragment.PrePass

#include "utility.frag"
#include "velocityCalc.frag"

layout(location = 1) out vec4 _normalAndVelocityOut;

void main() {
    _normalAndVelocityOut.rg = packNormal(VAR._normalWV);
    _normalAndVelocityOut.ba = velocityCalc(dvd_InvProjectionMatrix, getScreenPositionNormalised());
}