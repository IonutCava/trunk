-- Fragment

#include "utility.cmn"

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
layout(binding = TEXTURE_UNIT1) uniform sampler2D texDepth;

uniform vec2 size;
uniform float focalDepth = 0.5f;  //external focal point value, but you may use autofocus option below
uniform bool autofocus = true;    //use autofocus in shader? disable if you use external focalDepth value

out vec4 _colourOut;

float width = size.x; //texture width
float height = size.y; //texture height

vec2 texel = vec2(1.0 / width, 1.0 / height);

//------------------------------------------
//user variables

int samples = 3; //samples on the first ring
int rings = 5; //ring count

vec2 focus = vec2(0.5,0.5); // autofocus point on screen (0.0,0.0 - left lower corner, 1.0,1.0 - upper right)
float range = 4.0; //focal range
float maxblur = 1.25; //clamp value of max blur

float threshold = 0.5; //highlight threshold;
float gain = 10.0; //highlight gain;

float bias = 0.4; //bokeh edge bias
float fringe = 0.5; //bokeh chromatic aberration/fringing

bool noise = true; //use noise instead of pattern for sample dithering
float namount = 0.0001; //dither amount

bool depthblur = true; //blur the depth buffer?
float dbsize = 2.0; //depthblursize

/*
next part is experimental
not looking good with small sample and ring count
looks okay starting from samples = 4, rings = 4
*/

bool pentagon = false; //use pentagon as bokeh shape?
float feather = 0.4; //pentagon shape feather

//------------------------------------------


float penta(vec2 coords) //pentagonal shape
{
    float scale = float(rings) - 1.3;
    vec4  HS0 = vec4( 1.0,         0.0,         0.0,  1.0);
    vec4  HS1 = vec4( 0.309016994, 0.951056516, 0.0,  1.0);
    vec4  HS2 = vec4(-0.809016994, 0.587785252, 0.0,  1.0);
    vec4  HS3 = vec4(-0.809016994,-0.587785252, 0.0,  1.0);
    vec4  HS4 = vec4( 0.309016994,-0.951056516, 0.0,  1.0);
    vec4  HS5 = vec4( 0.0        ,0.0         , 1.0,  1.0);
    
    vec4  one = vec4( 1.0 );
    
    vec4 P = vec4((coords),vec2(scale, scale)); 
    
    vec4 dist = vec4(0.0);
    float inorout = -4.0;
    
    dist.x = dot( P, HS0 );
    dist.y = dot( P, HS1 );
    dist.z = dot( P, HS2 );
    dist.w = dot( P, HS3 );
    
    dist = smoothstep( -feather, feather, dist );
    
    inorout += dot( dist, one );
    
    dist.x = dot( P, HS4 );
    dist.y = HS5.w - abs( P.z );
    
    dist = smoothstep( -feather, feather, dist );
    inorout += dist.x;
    
    return clamp( inorout, 0.0, 1.0 );
}

float bdepth(vec2 coords) //blurring depth
{
    float d = 0.0;
    float kernel[9];
    vec2 offset[9];
    
    vec2 wh = vec2(texel.x, texel.y) * dbsize;
    
    offset[0] = vec2(-wh.x,-wh.y);
    offset[1] = vec2( 0.0, -wh.y);
    offset[2] = vec2( wh.x -wh.y);
    
    offset[3] = vec2(-wh.x,  0.0);
    offset[4] = vec2( 0.0,   0.0);
    offset[5] = vec2( wh.x,  0.0);
    
    offset[6] = vec2(-wh.x, wh.y);
    offset[7] = vec2( 0.0,  wh.y);
    offset[8] = vec2( wh.x, wh.y);
    
    kernel[0] = 1.0/16.0;   kernel[1] = 2.0/16.0;   kernel[2] = 1.0/16.0;
    kernel[3] = 2.0/16.0;   kernel[4] = 4.0/16.0;   kernel[5] = 2.0/16.0;
    kernel[6] = 1.0/16.0;   kernel[7] = 2.0/16.0;   kernel[8] = 1.0/16.0;
    
    
    for( int i=0; i<9; i++ )
    {
        float tmp = texture(texDepth, coords + offset[i]).r;
        d += tmp * kernel[i];
    }
    
    return d;
}


vec3 colour(vec2 coords,float blur) //processing the sample
{
    vec3 col = vec3(0.0);
    
    col.r = texture(texScreen,coords + vec2(0.0,1.0)*texel*fringe*blur).r;
    col.g = texture(texScreen,coords + vec2(-0.866,-0.5)*texel*fringe*blur).g;
    col.b = texture(texScreen,coords + vec2(0.866,-0.5)*texel*fringe*blur).b;
    
    vec3 lumcoeff = vec3(0.299,0.587,0.114);
    float lum = dot(col.rgb, lumcoeff);
    float thresh = max((lum-threshold)*gain, 0.0);
    return col+mix(vec3(0.0),col,thresh*blur);
}

vec2 rand(in vec2 coord) //generating noise/pattern texture for dithering
{
    float noiseX = ((fract(1.0-coord.s*(width/2.0))*0.25)+(fract(coord.t*(height/2.0))*0.75))*2.0-1.0;
    float noiseY = ((fract(1.0-coord.s*(width/2.0))*0.75)+(fract(coord.t*(height/2.0))*0.25))*2.0-1.0;
    
    if (noise)
    {
        noiseX = clamp(fract(sin(dot(coord ,vec2(12.9898,78.233))) * 43758.5453),0.0,1.0)*2.0-1.0;
        noiseY = clamp(fract(sin(dot(coord ,vec2(12.9898,78.233)*2.0)) * 43758.5453),0.0,1.0)*2.0-1.0;
    }
    return vec2(noiseX,noiseY);
}

void main() 
{
    
    float depth = texture(texDepth, VAR._texCoord).x;
    float blur = 0.0;
    
    if (depthblur)
    {
        depth = bdepth(VAR._texCoord);
    }
    
    blur = clamp((abs(depth - focalDepth)/range)*100.0,-maxblur,maxblur);
    
    if (autofocus)
    {
        float fDepth = texture(texDepth,focus).x;
        blur = clamp((abs(depth - fDepth)/range)*100.0,-maxblur,maxblur);
    }
    
    vec2 noise = rand(VAR._texCoord)*namount*blur;
    
    float w = (1.0/width)*blur+noise.x;
    float h = (1.0/height)*blur+noise.y;
    
    vec3 col = texture(texScreen, VAR._texCoord).rgb;
    float s = 1.0;
    
    int ringsamples;
    
    for (int i = 1; i <= rings; i += 1)
    {   
        ringsamples = i * samples;
         
        for (int j = 0 ; j < ringsamples ; j += 1)   
        {
            float step = M_PI *2.0f / float(ringsamples);
            float pw = (cos(float(j)*step)*float(i));
            float ph = (sin(float(j)*step)*float(i));
            float p = 1.0;
            if (pentagon)
            { 
            p = penta(vec2(pw,ph));
            }
            col += colour(VAR._texCoord + vec2(pw*w,ph*h),blur)*mix(1.0,(float(i))/(float(rings)),bias)*p;
            s += 1.0*mix(1.0,(float(i))/(float(rings)),bias)*p;   
        }
    }
    
    
    col /= s;   
    
    _colourOut = vec4(col, 1.0);
}
