-- Vertex

out vec4 _colour;
uniform mat4 dvd_WorldMatrix;

void main(){
  VAR._texCoord = inTexCoordData;
  _colour = inColourData;
  gl_Position = dvd_ViewProjectionMatrix * dvd_WorldMatrix * vec4(inVertexData,1.0);
} 

-- Fragment

#include "utility.frag"

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;

uniform bool useTexture;
in  vec4 _colour;
layout(location = 0) out vec4 _colourOut;

void main(){
    if(!useTexture){
        _colourOut = _colour;
    }else{
        _colourOut = texture(texDiffuse0, VAR._texCoord);
        _colourOut.rgb += _colour.rgb;
    }
}

--Vertex.GUI

out vec4 _colour;

void main() {
    VAR._texCoord = inTexCoordData;
    _colour = inColourData;
    gl_Position = dvd_ViewProjectionMatrix * vec4(inVertexData, 1.0);
}

-- Fragment.GUI

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;

in  vec4 _colour;

out vec4 _colourOut;

void main(){
    _colourOut = vec4(_colour.rgb, texture(texDiffuse0, VAR._texCoord).r);
}

--Vertex.EnvironmentProbe

#include "nodeBufferedInput.cmn"

uniform mat4 dvd_WorldMatrixOverride;

vec3 UNPACK_FLOAT(in float value) {
    return (fract(vec3(1.0, 256.0, 65536.0) * value)* 2.0) - 1.0;
}

void main(void) {

    VAR.dvd_instanceID = gl_BaseInstanceARB;

    vec4 dvd_Vertex = vec4(inVertexData, 1.0);
    vec3 dvd_Normal = UNPACK_FLOAT(inNormalData);
    VAR._vertexW = dvd_WorldMatrixOverride * dvd_Vertex;
    VAR._vertexWV = dvd_ViewMatrix * VAR._vertexW;
    VAR._normalWV = normalize(dvd_NormalMatrixWV(VAR.dvd_instanceID) * dvd_Normal);
    //Compute the final vert position
    gl_Position = dvd_ViewProjectionMatrix * VAR._vertexW;
}

--Fragment.EnvironmentProbe

#include "utility.frag"
#include "output.frag"

uniform uint dvd_LayerIndex;
layout(binding = TEXTURE_REFLECTION_CUBE) uniform samplerCubeArray texEnvironmentCube;

void main() {
    vec3 reflectDirection = reflect(normalize(VAR._vertexWV.xyz), VAR._normalWV);
    vec4 colour = vec4(texture(texEnvironmentCube, vec4(reflectDirection, dvd_LayerIndex)).rgb, 1.0);

    writeOutput(colour);
}