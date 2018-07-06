//Textures
//0 -> no texture
//1 -> use texDiffuse0
//n -> add texDiffuse0 + texDiffuse1 + ... texDiffuse(n-1)
uniform int textureCount;

uniform sampler2D texDiffuse0;
uniform sampler2D texDiffuse1;
uniform sampler2D texDiffuse2;

uniform int  textureOperation0;
uniform int  textureOperation1;
uniform int  textureOperation2;

uniform vec4 dvd_TextureEnvColor[3];

const int MODULATE   = 0;
const int ADD        = 1;
const int SUBSTRACT  = 2;
const int DIVIDE     = 3;
const int SMOOTH_ADD = 4;
const int SIGNED_ADD = 5;
const int COMBINE    = 6;
const int DECAL      = 7;
const int BLEND      = 8;
const int REPLACE    = 9;

void applyTexture(in sampler2D texUnit, in int type, in int index, in vec2 uv, inout vec4 color){

    // Read from the texture
    switch(type) {
        case REPLACE    : color  = texture(texUnit,uv);         break;
        case MODULATE   : color *= texture(texUnit,uv);         break;
        case SIGNED_ADD : color += (texture(texUnit,uv) - 0.5); break;
        case DIVIDE     : color /= texture(texUnit,uv);		    break;
        case SUBSTRACT  : color -= texture(texUnit,uv); 	    break;
        case DECAL      : {
            vec4 temp = texture(texUnit,uv);
            color = vec4(mix(color.rgb, temp.rgb, temp.a), color.a);
        }break;
        case BLEND      : {
            vec4 tex = texture(texUnit,uv);
            color = vec4(mix(color.rgb, dvd_TextureEnvColor[index].rgb, tex.rgb), color.a * tex.a);
        }break;
        case ADD        : {
            vec4 tex = texture(texUnit,uv);
            color.rgb += tex.rgb;
            color.a   *= tex.a;
            color = clamp(color, 0.0, 1.0);
        }break;
        case SMOOTH_ADD : {
            vec4 tex = texture(texUnit,uv);
            color = (color + tex) - (color * tex);
        }break;
        
        default         : color = clamp(texture(texUnit,uv) * color, 0.0, 1.0); break;
    }
}