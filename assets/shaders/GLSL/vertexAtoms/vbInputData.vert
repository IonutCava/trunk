invariant gl_Position;

out vec4 _vertexW;
out vec2 _texCoord;

uniform int lodLevel = 0;

uniform mat4 dvd_WorldMatrix;//[MAX_INSTANCES];
uniform mat3 dvd_NormalMatrix;
uniform mat4 dvd_WorldViewMatrix;
uniform mat4 dvd_WorldViewMatrixInverse;
uniform mat4 dvd_WorldViewProjectionMatrix;


layout(std140) uniform dvd_MatrixBlock
{
   mat4 dvd_ProjectionMatrix;
   mat4 dvd_ViewMatrix;
   mat4 dvd_ViewProjectionMatrix;
   vec4 dvd_ViewPort;
};

#include "clippingPlanes.vert"
#if defined(USE_GPU_SKINNING)
#include "boneTransforms.vert"
#endif

vec4  dvd_Vertex;
vec4  dvd_Color;
vec3  dvd_Normal;
vec3  dvd_Tangent;
vec3  dvd_BiTangent;

void computeData(){

    dvd_Vertex     = vec4(inVertexData,1.0);
    dvd_Normal     = inNormalData;
    dvd_Tangent    = inTangentData;
    dvd_BiTangent  = inBiTangentData;
    dvd_Color      = inColorData;

    #if defined(USE_GPU_SKINNING)
    applyBoneTransforms(dvd_Vertex, dvd_Normal, lodLevel);
    #endif

    _texCoord = inTexCoordData;
    _vertexW  = dvd_WorldMatrix * dvd_Vertex;

    setClipPlanes(_vertexW);
}




/*
static.frag
by Spatial
05 July 2013
*/

// A single iteration of Bob Jenkins' One-At-A-Time hashing algorithm.
uint hash(uint x) {
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}

// Compound versions of the hashing algorithm I whipped together.
uint hash(uvec2 v) { return hash(v.x ^ hash(v.y)); }
uint hash(uvec3 v) { return hash(v.x ^ hash(v.y) ^ hash(v.z)); }
uint hash(uvec4 v) { return hash(v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w)); }

// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
float floatConstruct(uint m) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0

    float  f = uintBitsToFloat(m);       // Range [1:2]
    return f - 1.0;                        // Range [0:1]
}

// Pseudo-random value in half-open range [0:1].
float random(float x) { return floatConstruct(hash(floatBitsToUint(x))); }
float random(vec2  v) { return floatConstruct(hash(floatBitsToUint(v))); }
float random(vec3  v) { return floatConstruct(hash(floatBitsToUint(v))); }
float random(vec4  v) { return floatConstruct(hash(floatBitsToUint(v))); }