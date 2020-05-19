-- Fragment.Generic

out vec4 _colourOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
uniform vec2 size;
uniform int kernelSize;
uniform int layer;

uniform bool layered = false;
uniform bool verticalBlur = false;

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
    if (layered) {
        if (verticalBlur) {
            _colourOut = vec4(blurVerticalLayered(), 1.0);
        } else {
            _colourOut = vec4(blurHorizontalLayered(), 1.0);
        }
    } else {
        if (verticalBlur) {
            _colourOut = vec4(blurVertical(), 1.0);
        } else {
            _colourOut = vec4(blurHorizontal(), 1.0);
        }
    }
}

-- Vertex.GaussBlur

void main(void)
{
}

--Geometry.GaussBlur

layout(points, invocations = MAX_CSM_SPLITS_PER_LIGHT) in;
layout(triangle_strip, max_vertices = 4) out;

uniform vec2 blurSizes[MAX_CSM_SPLITS_PER_LIGHT];
uniform int layerCount;
uniform int layerOffsetRead;
uniform int layerOffsetWrite;

uniform bool verticalBlur = false;

layout(location = 0) out flat int _blurred;
layout(location = 1) out vec3 _blurCoords[7];

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
    _blurCoords[1] = vec3(1.0, 1.0, layer);
    _blurCoords[2] = vec3(1.0, 1.0, layer);
    _blurCoords[3] = vec3(1.0, 1.0, layer);
    _blurCoords[4] = vec3(1.0, 1.0, layer);
    _blurCoords[5] = vec3(1.0, 1.0, layer);
    _blurCoords[6] = vec3(1.0, 1.0, layer);
    _blurred = 0;
}

void BlurRoutine(in float texCoordX, in float texCoordY, in int layer) {
    if (verticalBlur) {
        computeCoordsV(texCoordX, texCoordY, layer);
    } else {
        computeCoordsH(texCoordX, texCoordY, layer);
    }
}

void main() {
    if (gl_InvocationID < layerCount) {
        gl_Layer = gl_InvocationID + layerOffsetWrite;
        gl_Position = vec4(1.0, 1.0, 0.0, 1.0);
        BlurRoutine(1.0, 1.0, gl_InvocationID + layerOffsetRead);
        EmitVertex();

        gl_Layer = gl_InvocationID + layerOffsetWrite;
        gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);
        BlurRoutine(0.0, 1.0, gl_InvocationID + layerOffsetRead);
        EmitVertex();

        gl_Layer = gl_InvocationID + layerOffsetWrite;
        gl_Position = vec4(1.0, -1.0, 0.0, 1.0);
        BlurRoutine(1.0, 0.0, gl_InvocationID + layerOffsetRead);
        EmitVertex();

        gl_Layer = gl_InvocationID + layerOffsetWrite;
        gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);
        BlurRoutine(0.0, 0.0, gl_InvocationID + layerOffsetRead);
        EmitVertex();

        EndPrimitive();
    } else {
        gl_Layer = gl_InvocationID + layerOffsetWrite;
        gl_Position = vec4(1.0, 1.0, 0.0, 1.0);
        passThrough(1.0, 1.0, gl_InvocationID + layerOffsetRead);
        EmitVertex();

        gl_Layer = gl_InvocationID + layerOffsetWrite;
        gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);
        passThrough(0.0, 1.0, gl_InvocationID + layerOffsetRead);
        EmitVertex();

        gl_Layer = gl_InvocationID + layerOffsetWrite;
        gl_Position = vec4(1.0, -1.0, 0.0, 1.0);
        passThrough(1.0, 0.0, gl_InvocationID + layerOffsetRead);
        EmitVertex();

        gl_Layer = gl_InvocationID + layerOffsetWrite;
        gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);
        passThrough(0.0, 0.0, gl_InvocationID + layerOffsetRead);
        EmitVertex();

        EndPrimitive();
    }
}

--Fragment.GaussBlur

layout(location = 0) in flat int _blurred;
layout(location = 1) in vec3 _blurCoords[7];

out vec2 _outColour;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texScreen;

void main(void)
{
    if (_blurred == 1) {
        _outColour  = texture(texScreen, _blurCoords[0]).rg * 0.015625f; //(1.0  / 64.0);
        _outColour += texture(texScreen, _blurCoords[1]).rg * 0.09375f;  //(6.0  / 64.0);
        _outColour += texture(texScreen, _blurCoords[2]).rg * 0.234375f; //(15.0 / 64.0);
        _outColour += texture(texScreen, _blurCoords[3]).rg * 0.3125f;   //(20.0 / 64.0);
        _outColour += texture(texScreen, _blurCoords[4]).rg * 0.234375f; //(15.0 / 64.0);
        _outColour += texture(texScreen, _blurCoords[5]).rg * 0.09375f;  //(6.0  / 64.0);
        _outColour += texture(texScreen, _blurCoords[6]).rg * 0.015625f; //(1.0  / 64.0);
    } else {
        _outColour = texture(texScreen, _blurCoords[0]).rg;
    }
}


--Fragment.ObjectMotionBlur

//ref: http://john-chapman-graphics.blogspot.com/2013/01/per-object-motion-blur.html
layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
layout(binding = TEXTURE_UNIT1) uniform sampler2D texVelocity;

uniform float dvd_velocityScale;
uniform int dvd_maxSamples;

out vec4 _outColour;

void main(void) {
    const vec2 texelSize = 1.0f / vec2(textureSize(texScreen, 0));
    const vec2 screenTexCoords = gl_FragCoord.xy * texelSize;
    vec2 velocity = texture(texVelocity, screenTexCoords).ba * dvd_velocityScale;
    velocity.x = pow(velocity.x, 1.0f / 3.0f);
    velocity.y = pow(velocity.y, 1.0f / 3.0f);
    velocity * 2.0f - 1.0f;

    const int speed = int(length(velocity / texelSize));
    const int nSamples = clamp(speed, 1, dvd_maxSamples);

    _outColour = texture(texScreen, screenTexCoords);

    for (int i = 1; i < nSamples; ++i) {
        const vec2 offset = velocity * (float(i) / float(nSamples - 1) - 0.5f);
        _outColour += texture(texScreen, screenTexCoords + offset);
    }

    _outColour /= float(nSamples);
}