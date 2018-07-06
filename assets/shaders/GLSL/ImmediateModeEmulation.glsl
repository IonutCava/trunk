-- Vertex

out vec2 _texCoord;
out vec4 _color;
uniform mat4 dvd_WorldMatrix;

void main(){
  _texCoord = inTexCoordData;
  _color = inColorData;
  gl_Position = dvd_ViewProjectionMatrix * dvd_WorldMatrix * vec4(inVertexData,1.0);
} 

-- Vertex.GUI

out vec2 _texCoord;
out vec3 _color;

void main(){
  _texCoord = inTexCoordData;
  _color = inColorData.rgb;
  gl_Position = dvd_ProjectionMatrix * vec4(inVertexData,1.0);
} 

-- Fragment

#include "utility.frag"

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;

uniform bool useTexture;
in  vec4 _color;
out vec4 _colorOut;

void main(){
    if(!useTexture){
        _colorOut = _color;
    }else{
        _colorOut = texture(texDiffuse0, _texCoord);
        _colorOut.rgb += _color.rgb;
    }
	_colorOut = ToSRGB(_colorOut);
}

-- Fragment.GUI

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;

in  vec2 _texCoord;
in  vec3 _color;

out vec4 _colorOut;

void main(){
    _colorOut = vec4(_color, texture(texDiffuse0, _texCoord).r);
}