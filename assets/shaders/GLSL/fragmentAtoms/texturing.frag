#ifndef _TEXTURING_FRAG_
#define _TEXTURING_FRAG_
vec4 hash4(in vec2 p) {
    return fract(sin(vec4(1.0f + dot(p, vec2(37.0f, 17.0f)),
                          2.0f + dot(p, vec2(11.0f, 47.0f)),
                          3.0f + dot(p, vec2(41.0f, 29.0f)),
                          4.0f + dot(p, vec2(23.0f, 31.0f)))) * 103.0f);
}

vec4 textureNoTile(sampler2D samp, in vec2 uv)
{
    vec2 ddx = dFdx(uv);
    vec2 ddy = dFdy(uv);

    vec2 iuv = floor(uv);
    vec2 fuv = fract(uv);

    // generate per-tile transform
    vec4 ofa = hash4(iuv + vec2(0.0, 0.0));
    vec4 ofb = hash4(iuv + vec2(1.0, 0.0));
    vec4 ofc = hash4(iuv + vec2(0.0, 1.0));
    vec4 ofd = hash4(iuv + vec2(1.0f, 1.0f));

    // transform per-tile uvs
    ofa.zw = sign(ofa.zw - 0.5f);
    ofb.zw = sign(ofb.zw - 0.5f);
    ofc.zw = sign(ofc.zw - 0.5f);
    ofd.zw = sign(ofd.zw - 0.5f);

    // uv's, and derivarives (for correct mipmapping)
    vec2 uva = uv * ofa.zw + ofa.xy; vec2 ddxa = ddx * ofa.zw; vec2 ddya = ddy * ofa.zw;
    vec2 uvb = uv * ofb.zw + ofb.xy; vec2 ddxb = ddx * ofb.zw; vec2 ddyb = ddy * ofb.zw;
    vec2 uvc = uv * ofc.zw + ofc.xy; vec2 ddxc = ddx * ofc.zw; vec2 ddyc = ddy * ofc.zw;
    vec2 uvd = uv * ofd.zw + ofd.xy; vec2 ddxd = ddx * ofd.zw; vec2 ddyd = ddy * ofd.zw;

    // fetch and blend
    vec2 b = smoothstep(0.25f, 0.75f, fuv);

    return mix(mix(textureGrad(samp, uva, ddxa, ddya),
                   textureGrad(samp, uvb, ddxb, ddyb), b.x),
               mix(textureGrad(samp, uvc, ddxc, ddyc),
                   textureGrad(samp, uvd, ddxd, ddyd), b.x), b.y);
}

vec4 textureNoTile(sampler2DArray samp, in vec3 uvIn)
{
    const vec2 uv = uvIn.xy;

    vec2 ddx = dFdx(uv);
    vec2 ddy = dFdy(uv);

    vec2 iuv = floor(uv);
    vec2 fuv = fract(uv);

    // generate per-tile transform
    vec4 ofa = hash4(iuv + vec2(0.0f, 0.0f));
    vec4 ofb = hash4(iuv + vec2(1.0f, 0.0f));
    vec4 ofc = hash4(iuv + vec2(0.0f, 1.0f));
    vec4 ofd = hash4(iuv + vec2(1.0f, 1.0f));

    // transform per-tile uvs
    ofa.zw = sign(ofa.zw - 0.5f);
    ofb.zw = sign(ofb.zw - 0.5f);
    ofc.zw = sign(ofc.zw - 0.5f);
    ofd.zw = sign(ofd.zw - 0.5f);

    // uv's, and derivarives (for correct mipmapping)
    vec2 uva = uv * ofa.zw + ofa.xy; vec2 ddxa = ddx * ofa.zw; vec2 ddya = ddy * ofa.zw;
    vec2 uvb = uv * ofb.zw + ofb.xy; vec2 ddxb = ddx * ofb.zw; vec2 ddyb = ddy * ofb.zw;
    vec2 uvc = uv * ofc.zw + ofc.xy; vec2 ddxc = ddx * ofc.zw; vec2 ddyc = ddy * ofc.zw;
    vec2 uvd = uv * ofd.zw + ofd.xy; vec2 ddxd = ddx * ofd.zw; vec2 ddyd = ddy * ofd.zw;

    // fetch and blend
    vec2 b = smoothstep(0.25f, 0.75f, fuv);

    return mix(mix(textureGrad(samp, vec3(uva, uvIn.z), ddxa, ddya),
                   textureGrad(samp, vec3(uvb, uvIn.z), ddxb, ddyb), b.x),
               mix(textureGrad(samp, vec3(uvc, uvIn.z), ddxc, ddyc),
                   textureGrad(samp, vec3(uvd, uvIn.z), ddxd, ddyd), b.x), b.y);
}

#endif //_TEXTURING_FRAG_