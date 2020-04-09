-- Vertex

layout(location = 13) in vec4 particleNormalData;
// Output data will be interpolated for each fragment.
layout(location = 0) out vec4 particleColour;

void main()
{
    vec3 camRighW = dvd_ViewMatrix[0].xyz;
    vec3 camUpW = dvd_ViewMatrix[1].xyz;

    float spriteSize = particleNormalData.w;
    vec3 vertexPositionWV = dvd_ViewMatrix * vec4( particleNormalData.xyz + 
                                                  (camRighW * inVertexData.x * spriteSize) +
                                                  (camUpW * inVertexData.y * spriteSize), 1.0f);
    // Output position of the vertex
    // Even though the variable ends with WV, we'll store WVP to skip adding a new varying variable

    //VAR._vertexWV is actually VAR._vertexWVP
    VAR._vertexWV = dvd_ProjectionMatrix * vec4(vertexPositionWV, 1.0f);
    gl_Position = VAR._vertexWV;
    
    // UV of the vertex. No special space for this one.
    VAR._texCoord = inVertexData.xy + vec2(0.5, 0.5);

    particleColour = inColourData;
}

-- Fragment.Shadow

#ifdef HAS_TEXTURE
layout(location = 0) in vec4 particleColour;

void main(){
    colour = particleColour * texture(texDiffuse0, VAR._texCoord);
    if (colour.a < 1.0f - Z_TEST_SIGMA) {
        discard;
    }
}
#endif


--Fragment.Shadow.VSM

#ifdef HAS_TEXTURE
#if !defined(USE_SEPARATE_VSM_PASS)
#include "vsm.frag"
out vec2 _colourOut;
#endif

layout(location = 0) in vec4 particleColour;

void main() {
    colour = particleColour * texture(texDiffuse0, VAR._texCoord);
    if (colour.a < 1.0f - Z_TEST_SIGMA) {
        discard;
    }
#if !defined(USE_SEPARATE_VSM_PASS)
    _colourOut = computeMoments();
#endif
}
#endif

-- Fragment

#include "utility.frag"
#include "output.frag"

// Interpolated values from the vertex shaders
layout(location = 0) in vec4 particleColour;

#ifdef HAS_TEXTURE
layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;
#endif

layout(binding = TEXTURE_DEPTH_MAP) uniform sampler2D texDepthMap;

void main(){
   
#ifdef HAS_TEXTURE
    colour = particleColour * texture(texDiffuse0, VAR._texCoord);
#else
    colour = particleColour;
#endif

    float d = texture(texDepthMap, gl_FragCoord.xy * ivec2(dvd_ViewPort.zw)).r - gl_FragCoord.z;
    float softness = pow(1.0 - min(1.0, 200.0 * d), 2.0);
    colour.a *= max(0.1, 1.0 - pow(softness, 2.0));
    writeOutput(colour);

}