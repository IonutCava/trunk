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

-- Fragment.PrePass
// Ouput data
out vec4 colour;

void main(){
    //VAR._vertexWV is actually VAR._vertexWVP
    float depth = (VAR._vertexWV.z / VAR._vertexWV.w) * 0.5 + 0.5;
    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    colour = vec4(depth, (depth * depth) + 0.25 * (dx * dx + dy * dy), 0.0, 0.0);
}

-- Fragment

#include "utility.frag"
#include "velocityCalc.frag"

// Interpolated values from the vertex shaders
in vec4 particleColour;

// Ouput data
layout(location = 0) out vec4 colour;
layout(location = 1) out vec2 normal;
layout(location = 2) out vec2 velocity;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;

void main(){
   
#ifdef HAS_TEXTURE
    colour = particleColour * texture(texDiffuse0, VAR._texCoord);
#else
    colour = particleColour;
#endif

    float d = texture(texDepthMap, gl_FragCoord.xy * ivec2(dvd_ViewPort.zw)).r - gl_FragCoord.z;
    float softness = pow(1.0 - min(1.0, 200.0 * d), 2.0);
    colour.a *= max(0.1, 1.0 - pow(softness, 2.0));
    normal = packNormal(vec3(0.0, 0.0, 1.0));
    velocity = velocityCalc(dvd_InvProjectionMatrix, getScreenPositionNormalised());
}