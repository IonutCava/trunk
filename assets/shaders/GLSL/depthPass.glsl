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

void main(void)
{
    computeData();
}

-- Fragment.GaussBlur

in vec2 _texCoord;
out vec2 _outColor;

uniform sampler2DArray shadowMap;
uniform float blurSize; 
uniform bool horizontal = true;
uniform int layer;

void main(void)
{
    vec2 sum = vec2(0.0);
    if (horizontal){
        sum += texture(shadowMap, vec3(_texCoord.x, _texCoord.y - 4.0*blurSize, layer)).rg * 0.05;
        sum += texture(shadowMap, vec3(_texCoord.x, _texCoord.y - 3.0*blurSize, layer)).rg * 0.09;
        sum += texture(shadowMap, vec3(_texCoord.x, _texCoord.y - 2.0*blurSize, layer)).rg * 0.12;
        sum += texture(shadowMap, vec3(_texCoord.x, _texCoord.y - blurSize, layer)).rg * 0.15;
        sum += texture(shadowMap, vec3(_texCoord.x, _texCoord.y, layer)).rg * 0.16;
        sum += texture(shadowMap, vec3(_texCoord.x, _texCoord.y + blurSize, layer)).rg * 0.15;
        sum += texture(shadowMap, vec3(_texCoord.x, _texCoord.y + 2.0*blurSize, layer)).rg * 0.12;
        sum += texture(shadowMap, vec3(_texCoord.x, _texCoord.y + 3.0*blurSize, layer)).rg * 0.09;
        sum += texture(shadowMap, vec3(_texCoord.x, _texCoord.y + 4.0*blurSize, layer)).rg * 0.05;
    }else{
        sum += texture(shadowMap, vec3(_texCoord.x - 4.0*blurSize, _texCoord.y, layer)).rg * 0.05;
        sum += texture(shadowMap, vec3(_texCoord.x - 3.0*blurSize, _texCoord.y, layer)).rg * 0.09;
        sum += texture(shadowMap, vec3(_texCoord.x - 2.0*blurSize, _texCoord.y, layer)).rg * 0.12;
        sum += texture(shadowMap, vec3(_texCoord.x - blurSize, _texCoord.y, layer)).rg * 0.15;
        sum += texture(shadowMap, vec3(_texCoord.x, _texCoord.y, layer)).rg * 0.16;
        sum += texture(shadowMap, vec3(_texCoord.x + blurSize,     _texCoord.y, layer)).rg * 0.15;
        sum += texture(shadowMap, vec3(_texCoord.x + 2.0*blurSize, _texCoord.y, layer)).rg * 0.12;
        sum += texture(shadowMap, vec3(_texCoord.x + 3.0*blurSize, _texCoord.y, layer)).rg * 0.09;
        sum += texture(shadowMap, vec3(_texCoord.x + 4.0*blurSize, _texCoord.y, layer)).rg * 0.05;
    }

    _outColor = sum;
}