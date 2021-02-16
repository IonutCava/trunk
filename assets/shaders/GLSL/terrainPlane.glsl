--Vertex.PrePass

#include "vbInputData.vert"

layout(location = 0) out vec4 _scrollingUV;

void main(void) {
    const vec4 vertexWVP = computeData(fetchInputData());
    _scrollingUV = vec4(0.f);
    setClipPlanes();
    gl_Position = vertexWVP;
}

-- Vertex.Colour

#include "vbInputData.vert"

layout(location = 0) out vec4 _scrollingUV;

void main(void) {
    const vec4 vertexWVP = computeData(fetchInputData());
    setClipPlanes();

    const float time2 = MSToSeconds(dvd_time) * 0.01f;
    vec2 uvNormal0 = VAR._texCoord * UNDERWATER_TILE_SCALE;
    _scrollingUV.s += time2;
    _scrollingUV.t += time2;
    vec2 uvNormal1 = VAR._texCoord * UNDERWATER_TILE_SCALE;
    _scrollingUV.p -= time2;
    _scrollingUV.q += time2;

    gl_Position = vertexWVP;
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

    writeScreenColour(colourOut, VAR._normalWV);
}