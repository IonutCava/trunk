-- Vertex

out vec4 _color;
uniform mat4 dvd_WorldMatrix;

void main(){
  VAR._texCoord = inTexCoordData;
  _color = inColorData;
  gl_Position = dvd_ViewProjectionMatrix * dvd_WorldMatrix * vec4(inVertexData,1.0);
} 

-- Fragment

#include "utility.frag"

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;

uniform bool useTexture;
in  vec4 _color;
layout(location = 0) out vec4 _colorOut;
layout(location = 1) out vec3 _normalOut;

void main(){
    if(!useTexture){
        _colorOut = _color;
    }else{
        _colorOut = texture(texDiffuse0, VAR._texCoord);
        _colorOut.rgb += _color.rgb;
    }
    _colorOut = ToSRGB(_colorOut);
}

-- Fragment.GUI

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;

in  vec4 _color;

out vec4 _colorOut;

void main(){
    _colorOut = vec4(_color.rgb, texture(texDiffuse0, VAR._texCoord).r);
}