-- Vertex

in int  inSkipMVP;

out vec2 _texCoord;
out vec4 _color;

uniform mat4 dvd_WorldViewProjectionMatrix;

void main(){
  _texCoord = inTexCoordData;
  _color = inColorData;
  if (inSkipMVP < 1)
      gl_Position = dvd_WorldViewProjectionMatrix * vec4(inVertexData,1.0);
  else
      gl_Position = vec4(inVertexData, 1.0);
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

uniform sampler2D tex;
uniform bool useTexture;

in  vec2 _texCoord;
in  vec4 _color;
out vec4 _colorOut;

void main(){
    if(!useTexture){
        _colorOut = _color;
    }else{
        _colorOut = texture(tex, _texCoord);
        _colorOut.rgb += _color.rgb;
    }
}

-- Fragment.GUI

uniform sampler2D tex;

in  vec2 _texCoord;
in  vec3 _color;

out vec4 _colorOut;

void main(){
    _colorOut = vec4(_color, texture(tex, _texCoord).r);
}