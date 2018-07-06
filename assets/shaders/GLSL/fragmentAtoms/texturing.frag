//Textures
//0 -> no texture
//1 -> use texDiffuse0
//2 -> add texDiffuse0 + texDiffuse1
uniform int textureCount;
uniform sampler2D texDiffuse0;
uniform int       texDiffuse0Op;
uniform sampler2D texDiffuse1;
uniform int       texDiffuse1Op;

const int REPLACE    = 0;
const int MODULATE   = 1;
const int DECAL      = 2;
const int BLEND      = 3;
const int ADD        = 4;
const int SMOOTH_ADD = 5;
const int SIGNED_ADD = 6;
const int DIVIDE     = 7;
const int SUBSTRACT  = 8;
const int COMBINE    = 9;

void applyTexture2D(in sampler2D texUnit, in int type, in int index, in vec2 uv, inout vec4 color){
    // Read from the texture
    vec4 texture = texture2D(texUnit,uv);
   
    if (type == REPLACE){
        color = texture;

    } else if (type == MODULATE){
        color *= texture;

    } else if (type == DECAL){
        vec3 temp = mix(color.rgb, texture.rgb, texture.a);
        color = vec4(temp, color.a);

    }else if (type == BLEND) {
        vec3 temp = mix(color.rgb, gl_TextureEnvColor[index].rgb, texture.rgb);
        color = vec4(temp, color.a * texture.a);

    }else if (type == ADD){
        color.rgb += texture.rgb;
        color.a   *= texture.a;
        color = clamp(color, 0.0, 1.0);

	} else if (type == SMOOTH_ADD) {
		color = (color + texture) - (color * texture);

	} else if (type == SIGNED_ADD){
		color += (texture - 0.5f);

	}else if (type == DIVIDE){
		color /= texture;

	}else if (type == SUBSTRACT) {
		color -= texture;

	}else {
        color = clamp(texture * color, 0.0, 1.0);
    }
}