-- Vertex

layout(location = ATTRIB_POSITION) in vec3 inVertexData;
layout(location = ATTRIB_TEXCOORD) in vec2 inTexCoordData;
layout(location = ATTRIB_COLOR) in vec4 inColourData;
layout(location = ATTRIB_GENERIC) in vec2 inGenericData;

layout(location = 0) out vec2 Frag_UV;
layout(location = 1) out vec4 Frag_Color;

void main()
{
    Frag_UV = inTexCoordData;
    Frag_Color = inColourData;
    gl_Position = dvd_ViewProjectionMatrix * vec4(inGenericData,0,1);
};

-- Fragment

#include "utility.frag"

layout(binding = TEXTURE_UNIT0) uniform sampler2D Texture;

layout(location = 0) in vec2 Frag_UV;
layout(location = 1) in vec4 Frag_Color;

out vec4 Out_Color;

uniform int depthTexture;
uniform vec2 depthRange;
uniform ivec4 toggleChannel;

void main()
{
    Out_Color = Frag_Color;

    vec4 texColor = texture( Texture, Frag_UV.st);
    if (depthTexture == 1) {
        texColor = vec4(ToLinearDepth(texColor.r, dvd_zPlanes * depthRange) * toggleChannel[0]);
    } else {
        Out_Color *= toggleChannel;
    }
    
    Out_Color *= texColor;
};