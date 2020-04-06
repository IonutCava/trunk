-- Fragment

out vec4 _colourOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;

void main(){
    _colourOut = texture(texScreen VAR._texCoords);
}