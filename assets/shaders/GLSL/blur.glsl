-- Vertex
out vec2 _texCoord;

void main(void)
{
    vec2 uv = vec2(0,0);
    if((gl_VertexID & 1) != 0) {
        uv.x = 1;
    }

    if((gl_VertexID & 2) != 0) {
        uv.y = 1;
    }

    _texCoord = uv * 2;
    gl_Position.xy = uv * 4 - 1;
    gl_Position.zw = vec2(0,1);
}

-- Fragment

in  vec2 _texCoord;
out vec4 _colorOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
uniform vec2 size;
uniform int kernelSize;
uniform int layer;

subroutine vec3 BlurRoutineType();
subroutine uniform BlurRoutineType BlurRoutine;

subroutine(BlurRoutineType)
vec3 blurHorizontal(){
    vec2 pass = 1.0 / size;
    vec3 color = vec3(0.0);
    vec3 value;
    int sum = 0;
    int factor = 0;
    for (int i = -kernelSize; i <= kernelSize; i++) {
        value = texture(texScreen, _texCoord + vec2(pass.x*i, 0.0)).rgb;
        factor = kernelSize + 1 - abs(i);
        color += value * factor;
        sum += factor;
    }
    return color / sum;
}

subroutine(BlurRoutineType)
vec3 blurVertical(){
    vec2 pass = 1.0 / size;
    vec3 color = vec3(0.0);
    vec3 value;
    int sum = 0;
    int factor = 0;
    for (int i = -kernelSize; i <= kernelSize; i++) {
        value = texture(texScreen, _texCoord + vec2(0.0, pass.y*i)).rgb;
        factor = kernelSize + 1 - abs(i);
        color += value * factor;
        sum += factor;
    }
    return color / sum;
}

subroutine(BlurRoutineType)
vec3 blurHorizontalLayered(){
    vec2 pass = 1.0 / size;
    vec3 color = vec3(0.0);
    vec3 value;
    int sum = 0;
    int factor = 0;
    for (int i = -kernelSize; i <= kernelSize; i++) {
        value = texture(texScreen, _texCoord + vec2(pass.x*i, 0.0), layer).rgb;
        factor = kernelSize + 1 - abs(i);
        color += value * factor;
        sum += factor;
    }
    return color / sum;
}

subroutine(BlurRoutineType)
vec3 blurVerticalLayered(){
    vec2 pass = 1.0 / size;
    vec3 color = vec3(0.0);
    vec3 value;
    int sum = 0;
    int factor = 0;
    for (int i = -kernelSize; i <= kernelSize; i++) {
        value = texture(texScreen, _texCoord + vec2(0.0, pass.y*i), layer).rgb;
        factor = kernelSize + 1 - abs(i);
        color += value * factor;
        sum += factor;
    }
    return color / sum;
}

void main() {

    _colorOut = vec4(BlurRoutine(), 1.0);
}

-- Vertex.GaussBlur

void main(void)
{

}


--Geometry.GaussBlur

#define GS_MAX_INVOCATIONS 4

layout(points, invocations = GS_MAX_INVOCATIONS) in;
layout(triangle_strip, max_vertices = 4) out;

uniform vec2 blurSize;
uniform int layerCount;
uniform int layerOffset;

out vec3 _blurCoords[7];

subroutine void BlurRoutineType(in float texCoordX, in float texCoordY, in int layer);
subroutine uniform BlurRoutineType BlurRoutine;

subroutine(BlurRoutineType)
void computeCoordsH(in float texCoordX, in float texCoordY, in int layer){
    _blurCoords[0] = vec3(texCoordX, texCoordY - 3.0 * blurSize.x, layer);
    _blurCoords[1] = vec3(texCoordX, texCoordY - 2.0 * blurSize.x, layer);
    _blurCoords[2] = vec3(texCoordX, texCoordY - 1.0 * blurSize.x, layer);
    _blurCoords[3] = vec3(texCoordX, texCoordY,                    layer);
    _blurCoords[4] = vec3(texCoordX, texCoordY + 1.0 * blurSize.x, layer);
    _blurCoords[5] = vec3(texCoordX, texCoordY + 2.0 * blurSize.x, layer);
    _blurCoords[6] = vec3(texCoordX, texCoordY + 3.0 * blurSize.x, layer);
}

subroutine(BlurRoutineType)
void computeCoordsV(in float texCoordX, in float texCoordY, in int layer){
    _blurCoords[0] = vec3(texCoordX - 3.0*blurSize.y, texCoordY, layer);
    _blurCoords[1] = vec3(texCoordX - 2.0*blurSize.y, texCoordY, layer);
    _blurCoords[2] = vec3(texCoordX - 1.0*blurSize.y, texCoordY, layer);
    _blurCoords[3] = vec3(texCoordX,                  texCoordY, layer);
    _blurCoords[4] = vec3(texCoordX + 1.0*blurSize.y, texCoordY, layer);
    _blurCoords[5] = vec3(texCoordX + 2.0*blurSize.y, texCoordY, layer);
    _blurCoords[6] = vec3(texCoordX + 3.0*blurSize.y, texCoordY, layer);
}

void main() {
    if (gl_InvocationID >= layerOffset && gl_InvocationID < layerCount) {
        gl_Layer = gl_InvocationID;
        gl_Position = vec4(1.0, 1.0, 0.0, 1.0);
        BlurRoutine(1.0, 1.0, gl_Layer);
        EmitVertex();

        gl_Layer = gl_InvocationID;
        gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);
        BlurRoutine(0.0, 1.0, gl_Layer);
        EmitVertex();

        gl_Layer = gl_InvocationID;
        gl_Position = vec4(1.0, -1.0, 0.0, 1.0);
        BlurRoutine(1.0, 0.0, gl_Layer);
        EmitVertex();

        gl_Layer = gl_InvocationID;
        gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);
        BlurRoutine(0.0, 0.0, gl_Layer);
        EmitVertex();

        EndPrimitive();
    }
}

--Fragment.GaussBlur

in vec3 _blurCoords[7];

out vec2 _outColor;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texScreen;

void main(void)
{
    _outColor  = texture(texScreen, _blurCoords[0]).rg * (1.0  / 64.0);
    _outColor += texture(texScreen, _blurCoords[1]).rg * (6.0  / 64.0);
    _outColor += texture(texScreen, _blurCoords[2]).rg * (15.0 / 64.0);
    _outColor += texture(texScreen, _blurCoords[3]).rg * (20.0 / 64.0);
    _outColor += texture(texScreen, _blurCoords[4]).rg * (15.0 / 64.0);
    _outColor += texture(texScreen, _blurCoords[5]).rg * (6.0  / 64.0);
    _outColor += texture(texScreen, _blurCoords[6]).rg * (1.0  / 64.0);
}