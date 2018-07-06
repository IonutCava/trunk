-- Vertex
#include "foliage.vert"
#include "vbInputData.vert"
#include "lightingDefaults.vert"

layout(location = 10) in vec4 instanceData;
layout(location = 11) in float instanceScale;
layout(location = 12) in int  instanceID;
/*layout(std430, binding = 10) coherent readonly buffer dvd_transformBlock{
    mat4 transform[];
};*/

uniform vec3 positionOffsets[36];
uniform vec2 texCoordOffsets[36];
uniform mat3 rotationMatrices[18];
uniform float dvd_visibilityDistance;

flat out int _arrayLayer;

void main()
{
    VAR.dvd_drawID = gl_BaseInstanceARB;
    vec3 posOffset = positionOffsets[gl_VertexID];
    _arrayLayer = int(instanceData.w);
    VAR._texCoord = texCoordOffsets[gl_VertexID];
    VAR._vertexW = /* transform[gl_InstanceID]* */
               vec4(rotationMatrices[instanceID % 18] * 
                    positionOffsets[gl_VertexID] * 
                    instanceScale + 
                    instanceData.xyz, 
                    1.0);

    dvd_Normal = vec3(1.0, 1.0, 1.0);
    VAR._normalWV = dvd_NormalMatrixWV(VAR.dvd_drawID) * dvd_Normal;

    if (posOffset.y > 0.75) {
        computeFoliageMovementGrass(VAR._vertexW);
    }

    setClipPlanes(VAR._vertexW);

    //computeLightVectors();

    VAR._vertexVelocity = vec2(0.0, 0.0);

    gl_Position = dvd_ViewProjectionMatrix * VAR._vertexW;
}

-- Fragment

#include "BRDF.frag"

flat in int _arrayLayer;
layout(location = 0) out vec4 _colourOut;
layout(location = 1) out vec2 _normalOut;
layout(location = 2) out vec2 _velocityOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

void main (void){
    vec4 colour = texture(texDiffuseGrass, vec3(VAR._texCoord, _arrayLayer));
    if (colour.a < ALPHA_DISCARD_THRESHOLD) {
        discard;
    }
    //colour = getPixelColour(VAR._texCoord, VAR._normalWV);
    _colourOut = ToSRGB(applyFog(colour));
    _normalOut = packNormal(normalize(f_in._normalWV));
    _velocityOut = VAR._vertexVelocity;
}

--Fragment.Shadow

flat in int _arrayLayer;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

out vec2 _colourOut;

vec2 computeMoments(in float depth) {
    // Compute partial derivatives of depth.  
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    // Compute second moment over the pixel extents.  
    return vec2(depth, depth*depth + 0.25*(dx*dx + dy*dy));
}

void main(void){
    vec4 colour = texture(texDiffuseGrass, vec3(VAR._texCoord, _arrayLayer));
    if (colour.a < ALPHA_DISCARD_THRESHOLD) discard;

    _colourOut = computeMoments(gl_FragCoord.z);
}

--Fragment.PrePass

flat in int _arrayLayer;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuseGrass;

void main(void){
    vec4 colour = texture(texDiffuseGrass, vec3(VAR._texCoord, _arrayLayer));
    if (colour.a < ALPHA_DISCARD_THRESHOLD) discard;

}