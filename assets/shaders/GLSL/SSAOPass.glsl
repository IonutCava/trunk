-- Vertex

void main(void)
{
}

-- Geometry

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

out vec2 _texCoord;

void main() {
    gl_Position = vec4(1.0, 1.0, 0.0, 1.0);
    _texCoord = vec2(1.0, 1.0);
    EmitVertex();

    gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);
    _texCoord = vec2(0.0, 1.0);
    EmitVertex();

    gl_Position = vec4(1.0, -1.0, 0.0, 1.0);
    _texCoord = vec2(1.0, 0.0);
    EmitVertex();

    gl_Position = vec4(-1.0, -1.0, 0.0, 1.0);
    _texCoord = vec2(0.0, 0.0);
    EmitVertex();

    EndPrimitive();
}

-- Fragment

in vec2 _texCoord;

uniform sampler2D texDepth;
uniform sampler2D texScreen;
 
out vec4 _colorOut;

const float maxz = 0.00035;
const float div_factor = 3.0 ;
const float maxz2 =  maxz * div_factor;
 
float LinearizeDepth(in float z) {
    return (2.0 * dvd_ZPlanesCombined.x ) / (dvd_ZPlanesCombined.y + dvd_ZPlanesCombined.x - z * (dvd_ZPlanesCombined.y - dvd_ZPlanesCombined.x));
}
 
float CompareDepth(in float crtDepth, in vec2 uv1) {
    float dif1 = crtDepth - LinearizeDepth(texture( texDepth, uv1 ).x);
    return clamp(dif1, -maxz, maxz) -  clamp(dif1/div_factor,0.0,maxz2) ;
}
 
void main(void) {
    vec2 UV = _texCoord + vec2(0.0011); 
    float original_pix = LinearizeDepth(texture(texDepth, UV).x);
    float crtRealDepth = original_pix * dvd_ZPlanesCombined.y + dvd_ZPlanesCombined.x;
    
    float increment = 0.013 - clamp(original_pix * 0.4, 0.001, 0.009);  
 
    float dif1 = 0.0; 
    float dif2 = 0.0;
    float dif3 = 0.0;
    float dif4 = 0.0;
    float dif5 = 0.0;
    float dif6 = 0.0;
    float dif7 = 0.0;
    float dif8 = 0.0;
    float dif =  0.0;  

    float increment2 = increment * 0.75;
    vec2 inc1 = vec2(increment2,        0.0);
    vec2 inc2 = vec2(increment2 * 0.7, -increment2 * 0.9);
    vec2 inc3 = vec2(increment2  *0.7,  increment2 * 0.9);
 
    dif1 = CompareDepth(original_pix, UV - inc1);
    dif2 = CompareDepth(original_pix, UV + inc1);
    dif3 = CompareDepth(original_pix, UV + inc2);
    dif4 = CompareDepth(original_pix, UV - inc2);
    dif5 = CompareDepth(original_pix, UV - inc3);
    dif6 = CompareDepth(original_pix, UV + inc3);  
    dif1 += dif2;  
    dif3 += dif4;
    dif5 += dif6;
    dif  = max(dif1, 0.0) + max(dif3, 0.0) + max(dif5, 0.0);

    increment2 += increment;
    inc1 = vec2(increment2,        0.0);
    inc2 = vec2(increment2 * 0.7, -increment2 * 0.9);
    inc3 = vec2(increment2 * 0.7,  increment2 * 0.9);
    dif1 = CompareDepth(original_pix, UV - inc1);
    dif2 = CompareDepth(original_pix, UV + inc1);
    dif3 = CompareDepth(original_pix, UV + inc2);
    dif4 = CompareDepth(original_pix, UV - inc2);
    dif5 = CompareDepth(original_pix, UV - inc3);
    dif6 = CompareDepth(original_pix, UV + inc3);
    dif1 += dif2;  
    dif3 += dif4;
    dif5 += dif6;
    dif  += (max(dif1, 0.0) + max(dif3, 0.0) + max(dif5, 0.0)) * 0.7;
 
    increment2 += increment * 1.25;
    inc1 = vec2(increment2,        0.0);
    inc2 = vec2(increment2 * 0.7, -increment2 * 0.9);
    inc3 = vec2(increment2 * 0.7,  increment2 * 0.9);
    dif1 = CompareDepth(original_pix, UV - inc1);
    dif2 = CompareDepth(original_pix, UV + inc1);
    dif3 = CompareDepth(original_pix, UV + inc2);
    dif4 = CompareDepth(original_pix, UV - inc2);
    dif5 = CompareDepth(original_pix, UV - inc3);
    dif6 = CompareDepth(original_pix, UV + inc3);  
    dif1 += dif2;
    dif3 += dif4;
    dif5 += dif6 ;
    dif  += (max(dif1, 0.0) + max(dif3, 0.0) + max(dif5, 0.0)) * 0.5;

    _colorOut = (vec3(1.0 - dif * (dvd_ZPlanesCombined.y + dvd_ZPlanesCombined.x) * 255), 1.0);
}