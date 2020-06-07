--Vertex

#include "vbInputData.vert"

void main(void) {
    computeData(fetchInputData());
    setClipPlanes();
    gl_Position = VAR._vertexWVP;
}

-- Vertex.Colour

#include "vbInputData.vert"

layout(location = 0) out vec4 _scrollingUV;

void main(void) {
    computeData(fetchInputData());
    setClipPlanes();

    float time2 = float(dvd_time) * 0.0001;
    vec2 noiseUV = VAR._texCoord * UNDERWATER_TILE_SCALE;
    _scrollingUV.st = noiseUV;
    _scrollingUV.pq = noiseUV + time2;
    _scrollingUV.s -= time2;

    gl_Position = VAR._vertexWVP;
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
    NodeData data = dvd_Matrices[DATA_IDX];
    prepareData(data);

    writeOutput(data, 
                VAR._texCoord,
                getNormalWV(VAR._texCoord),
                vec3(0.0f));
}