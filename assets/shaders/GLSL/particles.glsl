-- Vertex

layout(location = 13) in vec4 particleNormalData;
// Output data will be interpolated for each fragment.
out vec4 particleColor;

// Values that stay constant for the whole mesh.
uniform vec3 CameraRight_worldspace;
uniform vec3 CameraUp_worldspace;

void main()
{
    float spriteSize = particleNormalData.w;
    vec3 vertexPosition_worldspace = particleNormalData.xyz + 
                                     (CameraRight_worldspace * (inVertexData.x * spriteSize)) +
                                     (CameraUp_worldspace * (inVertexData.y * spriteSize));
    // Output position of the vertex
    // Even though the variable ends with WV, we'll store WVP to skip adding a new varying variable
    VAR._vertexWV = dvd_ViewProjectionMatrix * vec4(vertexPosition_worldspace, 1.0f);
    gl_Position = VAR._vertexWV;
    
    // UV of the vertex. No special space for this one.
    VAR._texCoord = inVertexData.xy + vec2(0.5, 0.5);
    particleColor = inColorData;
}

-- Fragment.Depth
// Ouput data
out vec4 color;

void main(){
    //VAR._vertexWV is actually VAR._vertexWVP
    float depth = (VAR._vertexWV.z / VAR._vertexWV.w) * 0.5 + 0.5;
    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    color = vec4(depth, (depth * depth) + 0.25 * (dx * dx + dy * dy), 0.0, 0.0);
}

-- Fragment

// Interpolated values from the vertex shaders
in vec4 particleColor;

// Ouput data
layout(location = 0) out vec4 color;
layout(location = 1) out vec3 normal;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;
layout(binding = TEXTURE_DEPTH_MAP) uniform sampler2D texDepthMap;

void main(){
   
#ifdef HAS_TEXTURE
    color = particleColor * texture(texDiffuse0, VAR._texCoord);
#else
    color = particleColor;
#endif

    float d = texture(texDepthMap, gl_FragCoord.xy * ivec2(dvd_ViewPort.zw)).r - gl_FragCoord.z;
    float softness = pow(1.0 - min(1.0, 200.0 * d), 2.0);
    color.a *= max(0.1, 1.0 - pow(softness, 2.0));

    normal = vec3(0.0, 0.0, 1.0);
}