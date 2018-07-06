-- Vertex.Shadow

#include "clippingPlanes.vert"
#if defined(USE_GPU_SKINNING)
#include "boneTransforms.vert"
#endif

layout(std140) uniform dvd_MatrixBlock
{
    mat4 dvd_ProjectionMatrix;
    mat4 dvd_ViewMatrix;
    mat4 dvd_ViewProjectionMatrix;
};

uniform mat4 dvd_WorldMatrix;//[MAX_INSTANCES];
out float depth;
out vec2 _texCoord;

void main(void){
    vec4 dvd_Vertex = vec4(inVertexData, 1.0);

#if defined(USE_GPU_SKINNING)
    vec3 dvd_Normal = vec3(1.0);
    applyBoneTransforms(dvd_Vertex, dvd_Normal, 2);
#endif

    _texCoord = inTexCoordData;
    vec4 wVertex = dvd_WorldMatrix * dvd_Vertex;
    setClipPlanes(wVertex);
    gl_Position = dvd_ViewProjectionMatrix * wVertex;
    depth = (gl_Position.z / gl_Position.w);
}

-- Fragment.Shadow

#if defined(USE_OPACITY_MAP) || defined(USE_OPACITY_DIFFUSE_MAP)
in vec2 _texCoord;
#endif

#if defined(USE_OPACITY)
uniform float opacity = 1.0;
#elif defined(USE_OPACITY_MAP)
uniform sampler2D texOpacityMap;
#elif defined(USE_OPACITY_DIFFUSE_MAP)
uniform sampler2D texDiffuse0;
#elif defined(USE_OPACITY_DIFFUSE)
uniform mat4      material;
#endif

in  float depth;
out vec2 _colorOut;

void main(){

#if defined(USE_OPACITY_DIFFUSE)
    if (material[1].w < ALPHA_DISCARD_THRESHOLD) discard;
#elif defined(USE_OPACITY)
    if (opacity < ALPHA_DISCARD_THRESHOLD) discard;
#elif defined(USE_OPACITY_MAP)
    vec4 opacityMap = texture(texOpacityMap, _texCoord);
    if (max(min(opacityMap.r, opacityMap.g), min(opacityMap.b, opacityMap.a)) < ALPHA_DISCARD_THRESHOLD) discard;
#elif defined(USE_OPACITY_DIFFUSE_MAP)
    if (texture(texDiffuse0, _texCoord).a < ALPHA_DISCARD_THRESHOLD) discard;
#endif
    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    _colorOut = vec2(depth, (depth * depth) + 0.25*(dx*dx + dy*dy));
}

-- Vertex.PrePass

#include "vbInputData.vert"

void main() {

    computeData();    gl_Position = dvd_WorldViewProjectionMatrix * dvd_Vertex;}

-- Fragment.PrePass

in vec2 _texCoord;
in vec4 _vertexW;

uniform float opacity = 1.0;
uniform sampler2D texOpacityMap;
uniform sampler2D texDiffuse0;
uniform mat4      material;

out vec4 _colorOut;

void main(){

#if defined(USE_OPACITY_DIFFUSE)
    if (material[1].w < ALPHA_DISCARD_THRESHOLD) discard;
#elif defined(USE_OPACITY)
    if (opacity < ALPHA_DISCARD_THRESHOLD) discard;
#elif defined(USE_OPACITY_MAP)
    vec4 opacityMap = texture(texOpacityMap, _texCoord);
    if (max(min(opacityMap.r, opacityMap.g), min(opacityMap.b, opacityMap.a)) < ALPHA_DISCARD_THRESHOLD) discard;
#elif defined(USE_OPACITY_DIFFUSE_MAP)
    if (texture(texDiffuse0, _texCoord).a < ALPHA_DISCARD_THRESHOLD) discard;
#endif

    _colorOut = vec4(gl_FragCoord.w, 0.0, 0.0, 0.0);
}

-- Vertex.GaussBlur

#include "vertexDefault.vert"

uniform int layer;
uniform bool horizontal;
uniform float blurSize;

out vec3 _blurCoords[9];

void main(void)
{
    computeData();
    if (horizontal){
        _blurCoords[0] = vec3(_texCoord.x, _texCoord.y - 4.0*blurSize, layer);
        _blurCoords[1] = vec3(_texCoord.x, _texCoord.y - 3.0*blurSize, layer);
        _blurCoords[2] = vec3(_texCoord.x, _texCoord.y - 2.0*blurSize, layer);
        _blurCoords[3] = vec3(_texCoord.x, _texCoord.y - blurSize, layer);
        _blurCoords[4] = vec3(_texCoord.x, _texCoord.y, layer);
        _blurCoords[5] = vec3(_texCoord.x, _texCoord.y + blurSize, layer);
        _blurCoords[6] = vec3(_texCoord.x, _texCoord.y + 2.0*blurSize, layer);
        _blurCoords[7] = vec3(_texCoord.x, _texCoord.y + 3.0*blurSize, layer);
        _blurCoords[8] = vec3(_texCoord.x, _texCoord.y + 4.0*blurSize, layer);
    }
    else{
        _blurCoords[0] = vec3(_texCoord.x - 4.0*blurSize, _texCoord.y, layer);
        _blurCoords[1] = vec3(_texCoord.x - 3.0*blurSize, _texCoord.y, layer);
        _blurCoords[2] = vec3(_texCoord.x - 2.0*blurSize, _texCoord.y, layer);
        _blurCoords[3] = vec3(_texCoord.x - blurSize, _texCoord.y, layer);
        _blurCoords[4] = vec3(_texCoord.x, _texCoord.y, layer);
        _blurCoords[5] = vec3(_texCoord.x + blurSize, _texCoord.y, layer);
        _blurCoords[6] = vec3(_texCoord.x + 2.0*blurSize, _texCoord.y, layer);
        _blurCoords[7] = vec3(_texCoord.x + 3.0*blurSize, _texCoord.y, layer);
        _blurCoords[8] = vec3(_texCoord.x + 4.0*blurSize, _texCoord.y, layer);
    }
}

-- Fragment.GaussBlur

in vec2 _texCoord;
in vec3 _blurCoords[9];

out vec2 _outColor;

uniform sampler2DArray shadowMap;

void main(void)
{
    vec2 sum = vec2(0.0);
    sum += texture(shadowMap, _blurCoords[0]).rg * 0.05;
    sum += texture(shadowMap, _blurCoords[1]).rg * 0.09;
    sum += texture(shadowMap, _blurCoords[2]).rg * 0.12;
    sum += texture(shadowMap, _blurCoords[3]).rg * 0.15;
    sum += texture(shadowMap, _blurCoords[4]).rg * 0.16;
    sum += texture(shadowMap, _blurCoords[5]).rg * 0.15;
    sum += texture(shadowMap, _blurCoords[6]).rg * 0.12;
    sum += texture(shadowMap, _blurCoords[7]).rg * 0.09;
    sum += texture(shadowMap, _blurCoords[8]).rg * 0.05;

    _outColor = sum;
}