--Vertex

#include "vbInputData.vert"

void main(void) {

    computeData();
    gl_Position = dvd_ProjectionMatrix * VAR._vertexWV;
}

-- Vertex.Colour

#include "vbInputData.vert"

layout(location = 0) out vec4 _scrollingUV;

void main(void) {

    computeData();

    float time2 = float(dvd_time) * 0.0001;
    vec2 noiseUV = VAR._texCoord * UNDERWATER_TILE_SCALE;
    _scrollingUV.st = noiseUV;
    _scrollingUV.pq = noiseUV + time2;
    _scrollingUV.s -= time2;

    gl_Position = dvd_ProjectionMatrix * VAR._vertexWV;
}

--Fragment.Colour

layout(early_fragment_tests) in;

layout(location = 0) in vec4 _scrollingUV;

#include "utility.frag"
#include "output.frag"


layout(binding = TEXTURE_UNIT0) uniform sampler2D texWaterCaustics;


void main(void) {
    vec4 colourOut = (texture(texWaterCaustics, _scrollingUV.st) +
                      texture(texWaterCaustics, _scrollingUV.pq)) * 0.5;

    writeOutput(colourOut);
}

--Fragment.PrePass

#include "prePass.frag"

void main() {
    discard;
    outputNoVelocity(VAR._texCoord, 1.0f);
}