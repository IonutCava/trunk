-- Vertex

layout(location = 11) in float bladeScale;
layout(location = 12) in int   bladeTextureId;

uniform mat4 dvd_WorldViewProjectionMatrix;
uniform vec3 positionOffsets[36];
uniform vec2 texCoordOffsets[36];
uniform mat3 rotationMatrices[18];

out vec3 _texCoord;

void main()
{
    _texCoord = vec3(texCoordOffsets[gl_VertexID], bladeTextureId);
    vec3 currentBlade = rotationMatrices[gl_InstanceID % 18] * positionOffsets[gl_VertexID];
    currentBlade = currentBlade * bladeScale + inVertexData;
    gl_Position = dvd_WorldViewProjectionMatrix *  vec4(currentBlade, 1.0);
}

-- Fragment

in vec3 _texCoord;

out vec4 _colorOut;

uniform sampler2DArray texDiffuse;

void main (void){
    
    vec4 cBase = texture(texDiffuse, _texCoord);
    // SHADOW MAPPING
    float shadow = 1.0;
    //applyShadowDirectional(0, shadow);
    
    if (cBase.a < ALPHA_DISCARD_THRESHOLD) discard;
    _colorOut = cBase;
    //_colorOut.rgb = cBase.rgb *(0.2 + 0.8 * shadow);
   // _colorOut.a = _grassAlpha * cBase.a;
}
