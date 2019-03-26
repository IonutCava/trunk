#ifndef _BUMP_MAPPING_FRAG_
#define _BUMP_MAPPING_FRAG_

#include "lightInput.cmn"
#include "nodeBufferedInput.cmn"

//Normal or BumpMap
layout(binding = TEXTURE_NORMALMAP) uniform sampler2D texNormalMap;


mat3 getTBNMatrix() {
    return mat3(VAR._tangentWV, VAR._bitangentWV, VAR._normalWV);
}

vec3 getBump(in vec2 uv) {
    return normalize(2.0 * texture(texNormalMap, uv).rgb - 1.0);
}

// http://www.thetenthplanet.de/archives/1180
mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv)
{
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx(p);
    vec3 dp2 = dFdy(p);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);

    // solve the linear system
    vec3 dp2perp = cross(dp2, N);
    vec3 dp1perp = cross(N, dp1);
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    // construct a scale-invariant frame 
    float invmax = inversesqrt(max(dot(T, T), dot(B, B)));
    return mat3(T * invmax, B * invmax, N);
}

vec3 perturb_normal(vec3 texNorm, vec3 N, vec3 V, vec2 texcoord)
{
    // assume N, the interpolated vertex normal and 
    // V, the view vector (vertex to eye)
    texNorm = texNorm * 255. / 127. - 128. / 127.;
    mat3 TBN = cotangent_frame(N, -V, texcoord);
    return normalize(TBN * texNorm);
}


void aproximateTBN(in vec3 normalWV, out vec3 tangent, out vec3 bitangent) {
    vec3 c1 = cross(normalWV, vec3(0.0, 0.0, 1.0));
    vec3 c2 = cross(normalWV, vec3(0.0, 1.0, 0.0));
    if (length(c1) > length(c2)) {
        tangent = c1;
    } else {
        tangent = c2;
    }
    tangent = normalize(tangent);
    bitangent = normalize(cross(normalWV, tangent));
}
/*---------------- Normal blending -----------------*/
//ref: http://blog.selfshadow.com/sandbox/normals.html

vec3 normalLinearBlend(in vec3 n1, in vec3 n2) {
    return normalize(n1 + n2);
}

vec3 normalOverlayBlend(in vec3 n1, in vec3 n2) {
    /*n1 = texture2D(base_map,   uv).xyz;
    n2 = texture2D(detail_map, uv).xyz;

    vec3 n(overlay(n1.x, n2.x),
           overlay(n1.y, n2.y),
           overlay(n1.z, n2.z));

    return normalize(n*2.0 - 1.0);*/

    return normalLinearBlend(n1, n2);
}

vec3 normalPartialDerivativesBlend(in vec3 n1, in vec3 n2) {
    return normalize(vec3(n1.xy * n2.z + n2.xy * n1.z,
                         n1.z * n2.z));
}

vec3 normalWhiteoutBlend(in vec3 n1, in vec3 n2) {
    return normalize(vec3(n1.xy + n2.xy, n1.z*n2.z));
}

vec3 normalUDNBlend(in vec3 n1, in vec3 n2) {
    return normalize(vec3(n1.xy + n2.xy, n1.z));
}

vec3 normalRNMBlend(in vec3 n1, in vec3 n2) {
    vec3 n1_2 = (n1 + 1) * 0.5;
    vec3 n2_2 = (n2 + 1) * 0.5;

    n1_2 = n1_2 * vec3( 2,  2, 2) + vec3(-1, -1,  0);
    n2_2 = n2_2 * vec3(-2, -2, 2) + vec3( 1,  1, -1);
    return n1_2 * dot(n1_2, n2_2) / n1_2.z - n2_2;
}

vec3 normalUnityBlend(in vec3 n1, in vec3 n2) {
    mat3 nBasis = mat3(
        vec3(n1.z, n1.y, -n1.x), // +90 degree rotation around y axis
        vec3(n1.x, n1.z, -n1.y), // -90 degree rotation around x axis
        vec3(n1.x, n1.y,  n1.z));
    return normalize(n2.x*nBasis[0] + n2.y*nBasis[1] + n2.z*nBasis[2]);
}

#endif //_BUMP_MAPPING_FRAG_