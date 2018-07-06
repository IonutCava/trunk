-- Vertex

void main(void)
{
    vec2 uv = vec2(0,0);
    if((gl_VertexID & 1) != 0) {
        uv.x = 1;
    }

    if((gl_VertexID & 2) != 0) {
        uv.y = 1;
    }

    VAR._texCoord = uv * 2;
    gl_Position.xy = uv * 4 - 1;
    gl_Position.zw = vec2(0,1);
}

-- Fragment

out vec4 _colourOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
uniform vec2 size;
uniform int kernelSize;
uniform int layer;

subroutine vec3 BlurRoutineType();
subroutine uniform BlurRoutineType BlurRoutine;

subroutine(BlurRoutineType)
vec3 blurHorizontal(){
    vec2 pass = 1.0 / size;
    vec3 colour = vec3(0.0);
    vec3 value;
    int sum = 0;
    int factor = 0;
    for (int i = -kernelSize; i <= kernelSize; i++) {
        value = texture(texScreen, VAR._texCoord + vec2(pass.x*i, 0.0)).rgb;
        factor = kernelSize + 1 - abs(i);
        colour += value * factor;
        sum += factor;
    }
    return colour / sum;
}

subroutine(BlurRoutineType)
vec3 blurVertical(){
    vec2 pass = 1.0 / size;
    vec3 colour = vec3(0.0);
    vec3 value;
    int sum = 0;
    int factor = 0;
    for (int i = -kernelSize; i <= kernelSize; i++) {
        value = texture(texScreen, VAR._texCoord + vec2(0.0, pass.y*i)).rgb;
        factor = kernelSize + 1 - abs(i);
        colour += value * factor;
        sum += factor;
    }
    return colour / sum;
}

subroutine(BlurRoutineType)
vec3 blurHorizontalLayered(){
    vec2 pass = 1.0 / size;
    vec3 colour = vec3(0.0);
    vec3 value;
    int sum = 0;
    int factor = 0;
    for (int i = -kernelSize; i <= kernelSize; i++) {
        value = texture(texScreen, VAR._texCoord + vec2(pass.x*i, 0.0), layer).rgb;
        factor = kernelSize + 1 - abs(i);
        colour += value * factor;
        sum += factor;
    }
    return colour / sum;
}

subroutine(BlurRoutineType)
vec3 blurVerticalLayered(){
    vec2 pass = 1.0 / size;
    vec3 colour = vec3(0.0);
    vec3 value;
    int sum = 0;
    int factor = 0;
    for (int i = -kernelSize; i <= kernelSize; i++) {
        value = texture(texScreen, VAR._texCoord + vec2(0.0, pass.y*i), layer).rgb;
        factor = kernelSize + 1 - abs(i);
        colour += value * factor;
        sum += factor;
    }
    return colour / sum;
}

void main() {

    _colourOut = vec4(BlurRoutine(), 1.0);
}

-- Vertex.GaussBlur

void main(void)
{

}


--Geometry.GaussBlur

#define GS_MAX_INVOCATIONS MAX_SPLITS_PER_LIGHT

layout(points, invocations = GS_MAX_INVOCATIONS) in;
layout(triangle_strip, max_vertices = 4) out;

uniform vec2 blurSizes[GS_MAX_INVOCATIONS];
uniform int layerCount;
uniform int layerOffsetRead;
uniform int layerOffsetWrite;
uniform int layerTotalCount;

out vec3 _blurCoords[7];
out flat int _blurred;

subroutine void BlurRoutineType(in float texCoordX, in float texCoordY, in int layer);
subroutine uniform BlurRoutineType BlurRoutine;

subroutine(BlurRoutineType)
void computeCoordsH(in float texCoordX, in float texCoordY, in int layer){
    vec2 blurSize = blurSizes[layer];
    _blurCoords[0] = vec3(texCoordX, texCoordY - 3.0 * blurSize.y, layer);
    _blurCoords[1] = vec3(texCoordX, texCoordY - 2.0 * blurSize.y, layer);
    _blurCoords[2] = vec3(texCoordX, texCoordY - 1.0 * blurSize.y, layer);
    _blurCoords[3] = vec3(texCoordX, texCoordY,                    layer);
    _blurCoords[4] = vec3(texCoordX, texCoordY + 1.0 * blurSize.y, layer);
    _blurCoords[5] = vec3(texCoordX, texCoordY + 2.0 * blurSize.y, layer);
    _blurCoords[6] = vec3(texCoordX, texCoordY + 3.0 * blurSize.y, layer);
    _blurred = 1;
}

subroutine(BlurRoutineType)
void computeCoordsV(in float texCoordX, in float texCoordY, in int layer){
    vec2 blurSize = blurSizes[layer];
    _blurCoords[0] = vec3(texCoordX - 3.0 * blurSize.x, texCoordY, layer);
    _blurCoords[1] = vec3(texCoordX - 2.0 * blurSize.x, texCoordY, layer);
    _blurCoords[2] = vec3(texCoordX - 1.0 * blurSize.x, texCoordY, layer);
    _blurCoords[3] = vec3(texCoordX,                    texCoordY, layer);
    _blurCoords[4] = vec3(texCoordX + 1.0 * blurSize.x, texCoordY, layer);
    _blurCoords[5] = vec3(texCoordX + 2.0 * blurSize.x, texCoordY, layer);
    _blurCoords[6] = vec3(texCoordX + 3.0 * blurSize.x, texCoordY, layer);
    _blurred = 1;
}

void passThrough(in float texCoordX, in float texCoordY, in int layer) {
    _blurCoords[0] = vec3(texCoordX, texCoordY, layer);
    _blurred = 0;
}

void main() {
    if (gl_InvocationID < layerCount) {
        gl_Layer = gl_InvocationID + layerOffsetWrite;
        gl_Position = vec4(1.0, 1.0, 0.0, 1.0);
        BlurRoutine(1.0, 1.0, gl_Layer + layerOffsetRead);
        EmitVertex();

        gl_Layer = gl_InvocationID + layerOffsetWrite;
        gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);
        BlurRoutine(0.0, 1.0, gl_Layer + layerOffsetRead);
        EmitVertex();

        gl_Layer = gl_InvocationID + layerOffsetWrite;
        gl_Position = vec4(1.0, -1.0, 0.0, 1.0);
        BlurRoutine(1.0, 0.0, gl_Layer + layerOffsetRead);
        EmitVertex();

        gl_Layer = gl_InvocationID + layerOffsetWrite;
        gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);
        BlurRoutine(0.0, 0.0, gl_Layer + layerOffsetRead);
        EmitVertex();

        EndPrimitive();
    } else {
        gl_Layer = gl_InvocationID + layerOffsetWrite;
        gl_Position = vec4(1.0, 1.0, 0.0, 1.0);
        passThrough(1.0, 1.0, gl_Layer + layerOffsetRead);
        EmitVertex();

        gl_Layer = gl_InvocationID + layerOffsetWrite;
        gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);
        passThrough(0.0, 1.0, gl_Layer + layerOffsetRead);
        EmitVertex();

        gl_Layer = gl_InvocationID + layerOffsetWrite;
        gl_Position = vec4(1.0, -1.0, 0.0, 1.0);
        passThrough(1.0, 0.0, gl_Layer + layerOffsetRead);
        EmitVertex();

        gl_Layer = gl_InvocationID + layerOffsetWrite;
        gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);
        passThrough(0.0, 0.0, gl_Layer + layerOffsetRead);
        EmitVertex();

        EndPrimitive();
    }
}

--Fragment.GaussBlur

in vec3 _blurCoords[7];
in flat int _blurred;
out vec2 _outColour;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texScreen;

void main(void)
{
    if (_blurred == 1) {
        _outColour  = texture(texScreen, _blurCoords[0]).rg * (1.0  / 64.0);
        _outColour += texture(texScreen, _blurCoords[1]).rg * (6.0  / 64.0);
        _outColour += texture(texScreen, _blurCoords[2]).rg * (15.0 / 64.0);
        _outColour += texture(texScreen, _blurCoords[3]).rg * (20.0 / 64.0);
        _outColour += texture(texScreen, _blurCoords[4]).rg * (15.0 / 64.0);
        _outColour += texture(texScreen, _blurCoords[5]).rg * (6.0  / 64.0);
        _outColour += texture(texScreen, _blurCoords[6]).rg * (1.0  / 64.0);
    } else {
        _outColour = texture(texScreen, _blurCoords[0]).rg;
    }
}