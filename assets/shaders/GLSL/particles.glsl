-- Vertex

layout(location = 13) in vec4 particleNormalData;
// Output data ; will be interpolated for each fragment.
out vec2 texCoord;
out vec4 particleColor;
out vec4 vertexVP;

// Values that stay constant for the whole mesh.
uniform vec3 CameraRight_worldspace;
uniform vec3 CameraUp_worldspace;

void main()
{
    float particleSize = particleNormalData.w; // because we encoded it this way.
    vec3 vertexPosition_worldspace = particleNormalData.xyz + CameraRight_worldspace * inVertexData.x * particleSize
                                                            + CameraUp_worldspace * inVertexData.y * particleSize;
    // Output position of the vertex
    vertexVP = dvd_ViewProjectionMatrix * vec4(vertexPosition_worldspace, 1.0f);
    gl_Position = vertexVP;
    
    // UV of the vertex. No special space for this one.
    texCoord = inVertexData.xy + vec2(0.5, 0.5);
    particleColor = inColorData;
}


-- Fragment.Depth
// Interpolated values from the vertex shaders
in vec4 vertexVP;

// Ouput data
out vec4 color;

void main(){
    float depth = (vertexVP.z / vertexVP.w) * 0.5 + 0.5;    
    // Adjusting moments (this is sort of bias per pixel) using partial derivative
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    color = vec4(depth, (depth * depth) + 0.25 * (dx * dx + dy * dy), 0.0, 0.0);
}

-- Fragment

// Interpolated values from the vertex shaders
in vec2 texCoord;
in vec4 particleColor;
in vec4 vertexVP;

// Ouput data
out vec4 color;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texDiffuse0;
uniform sampler2D depthBuffer;
uniform ivec2 dvd_screenDimension;

void main(){
    
    float d = texture(depthBuffer, gl_FragCoord.xy * dvd_screenDimension).r  - gl_FragCoord.z;
    float softness = pow(1.0 - min(1.0, 40.0 * d), 2.0);
    softness = 1.0 - pow(softness, 2.0);

    color = texture(texDiffuse0, texCoord) * particleColor;
    color.a *= softness;
}