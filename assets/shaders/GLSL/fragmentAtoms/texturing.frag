#ifndef _TEXTURING_FRAG_
#define _TEXTURING_FRAG_

// The MIT License
// Copyright © 2015 Inigo Quilez
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


// One way to avoid texture tile repetition one using one small texture to cover a huge area.
// Based on Voronoise (https://www.shadertoy.com/view/Xd23Dh), a random offset is applied to
// the texture UVs per Voronoi cell. Distance to the cell is used to smooth the transitions
// between cells.

// More info here: http://www.iquilezles.org/www/articles/texturerepetition/texturerepetition.htm


vec4 hash4(in vec2 p) {
    return fract(sin(vec4(1.0f + dot(p, vec2(37.0f, 17.0f)),
                          2.0f + dot(p, vec2(11.0f, 47.0f)),
                          3.0f + dot(p, vec2(41.0f, 29.0f)),
                          4.0f + dot(p, vec2(23.0f, 31.0f)))) * 103.0f);
}

float sum(vec3 v) { return v.x + v.y + v.z; }

vec3 textureNoTile(sampler2D samp, in vec2 uv) {
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
                   textureGrad(samp, uvd, ddxd, ddyd), b.x), b.y).rgb;
}

vec3 textureNoTile(sampler2D samp, in vec2 uv, in float v) {
    vec2 p = floor(uv);
    vec2 f = fract(uv);

    // derivatives (for correct mipmapping)
    vec2 ddx = dFdx(uv);
    vec2 ddy = dFdy(uv);

    vec3 va = vec3(0.0);
    float w1 = 0.0;
    float w2 = 0.0;
    for (int j = -1; j <= 1; j++)
        for (int i = -1; i <= 1; i++)
        {
            vec2 g = vec2(float(i), float(j));
            vec4 o = hash4(p + g);
            vec2 r = g - f + o.xy;
            float d = dot(r, r);
            float w = exp(-5.0 * d);
            vec3 c = textureGrad(samp, uv + v * o.zw, ddx, ddy).xyz;
            va += w * c;
            w1 += w;
            w2 += w * w;
        }

    // normal averaging --> lowers contrasts
    //return va/w1;

    // contrast preserving average
    float mean = 0.3;// textureGrad( samp, uv, ddx*16.0, ddy*16.0 ).x;
    vec3 res = mean + (va - w1 * mean) / sqrt(w2);
    return mix(va / w1, res, v);
}

vec3 textureNoTile(sampler2D samp, sampler2DArray noiseSampler, in int noiseSamplerIdx, in vec2 uv) {
    // sample variation pattern    
    float k = texture(noiseSampler, vec3(0.005 * uv, noiseSamplerIdx)).x; // cheap (cache friendly) lookup    

    // compute index    
    float index = k * 8.0;
    float i = floor(index);
    float f = fract(index);

    // offsets for the different virtual patterns    
    vec2 offa = sin(vec2(3.0, 7.0) * (i + 0.0)); // can replace with any other hash    
    vec2 offb = sin(vec2(3.0, 7.0) * (i + 1.0)); // can replace with any other hash    

    // compute derivatives for mip-mapping    
    vec2 dx = dFdx(uv), dy = dFdy(uv);

    // sample the two closest virtual patterns    
    vec3 cola = textureGrad(samp, uv + offa, dx, dy).rgb;
    vec3 colb = textureGrad(samp, uv + offb, dx, dy).rgb;

    // interpolate between the two virtual patterns    
    return mix(cola, colb, smoothstep(0.2, 0.8, f - 0.1 * sum(cola - colb)));
}

vec3 textureNoTile(sampler2DArray samp, in vec3 uvIn) {
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
                   textureGrad(samp, vec3(uvd, uvIn.z), ddxd, ddyd), b.x), b.y).rgb;
}

//float v = smoothstep( 0.4, 0.6, sin(iTime    ) );
vec3 textureNoTile(sampler2DArray samp, in vec3 uvIn, float v) {
    const vec2 uv = uvIn.xy;

    vec2 p = floor(uv);
    vec2 f = fract(uv);

    // derivatives (for correct mipmapping)
    vec2 ddx = dFdx(uv);
    vec2 ddy = dFdy(uv);

    vec3 va = vec3(0.0);
    float w1 = 0.0;
    float w2 = 0.0;
    for (int j = -1; j <= 1; j++)
        for (int i = -1; i <= 1; i++)
        {
            vec2 g = vec2(float(i), float(j));
            vec4 o = hash4(p + g);
            vec2 r = g - f + o.xy;
            float d = dot(r, r);
            float w = exp(-5.0 * d);
            vec3 c = textureGrad(samp, vec3(uv + v * o.zw, uvIn.z), ddx, ddy).xyz;
            va += w * c;
            w1 += w;
            w2 += w * w;
        }

    // normal averaging --> lowers contrasts
    //return va/w1;

    // contrast preserving average
    float mean = 0.3;// textureGrad( samp, vec3(uv, uvIn.z), ddx*16.0, ddy*16.0 ).x;
    vec3 res = mean + (va - w1 * mean) / sqrt(w2);
    return mix(va / w1, res, v);
}

vec3 textureNoTile(sampler2DArray samp, sampler2DArray noiseSampler, in int noiseSamplerIdx, in vec3 uvIn) {
    const vec2 uv = uvIn.xy;

    // sample variation pattern    
    float k = texture(noiseSampler, vec3(0.005f * uv, noiseSamplerIdx)).x; // cheap (cache friendly) lookup    

    // compute index    
    float index = k * 8.0;
    float i = floor(index);
    float f = fract(index);

    // offsets for the different virtual patterns    
    vec2 offa = sin(vec2(3.0, 7.0) * (i + 0.0)); // can replace with any other hash    
    vec2 offb = sin(vec2(3.0, 7.0) * (i + 1.0)); // can replace with any other hash    

    // compute derivatives for mip-mapping    
    vec2 dx = dFdx(uv), dy = dFdy(uv);

    // sample the two closest virtual patterns    
    vec3 cola = textureGrad(samp, vec3(uv + offa, uvIn.z), dx, dy).rgb;
    vec3 colb = textureGrad(samp, vec3(uv + offb, uvIn.z), dx, dy).rgb;

    // interpolate between the two virtual patterns    
    return mix(cola, colb, smoothstep(0.2, 0.8, f - 0.1 * sum(cola - colb)));
}

#endif //_TEXTURING_FRAG_