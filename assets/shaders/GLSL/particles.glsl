-- Vertex

layout(location = 13) in vec4 particleNormalData;
// Output data will be interpolated for each fragment.
out vec4 particleColour;

void main()
{
    vec3 camRighW = dvd_ViewMatrix[0].xyz;
    vec3 camUpW = dvd_ViewMatrix[1].xyz;

    float spriteSize = particleNormalData.w;
    vec3 vertexPositionW = particleNormalData.xyz + 
                           (camRighW * inVertexData.x * spriteSize) +
                           (camUpW * inVertexData.y * spriteSize);
    // Output position of the vertex
    // Even though the variable ends with WV, we'll store WVP to skip adding a new varying variable
    VAR._vertexWV = dvd_ViewProjectionMatrix * vec4(vertexPositionW, 1.0f);
    gl_Position = VAR._vertexWV;
    
    // UV of the vertex. No special space for this one.
    VAR._texCoord = inVertexData.xy + vec2(0.5, 0.5);

    particleColour = inColourData;
}

-- Fragment.Shadow
// Ouput data
out vec2 colour;

void main(){
    //VAR._vertexWV is actually VAR._vertexWVP
    float depth = (VAR._vertexWV.z / VAR._vertexWV.w) * 0.5 + 0.5;
    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    colour = vec2(depth, pow(depth, 2.0) + 0.25 * (dx * dx + dy * dy));
}

-- Fragment

#include "utility.frag"
#include "output.frag"

// Interpolated values from the vertex shaders
in vec4 particleColour;

#ifdef HAS_TEXTURE
layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;
#endif

void main(){
   
#ifdef HAS_TEXTURE
    colour = particleColour * texture(texDiffuse0, VAR._texCoord);
#else
    colour = particleColour;
#endif

    float d = texture(texDepthMap, gl_FragCoord.xy * ivec2(dvd_ViewPort.zw)).r - gl_FragCoord.z;
    float softness = pow(1.0 - min(1.0, 200.0 * d), 2.0);
    colour.a *= max(0.1, 1.0 - pow(softness, 2.0));
    writeOutput(colour, packNormal(vec3(0.0, 0.0, 1.0)));

}