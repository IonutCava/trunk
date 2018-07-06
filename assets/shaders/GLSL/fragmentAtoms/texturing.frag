//Textures
//0 -> no texture
//1 -> use texDiffuse0
//2 -> add texDiffuse0 + texDiffuse1
uniform int textureCount;
uniform sampler2D texDiffuse0;
uniform int       texDiffuse0Op;
uniform sampler2D texDiffuse1;
uniform int       texDiffuse1Op;

#if defined(USE_OPACITY_MAP)
//Opacity and specular maps
uniform sampler2D opacityMap;
#endif
#if defined(USE_SPECULAR_MAP)
uniform sampler2D specularMap;
#endif

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

void applyTexture(in sampler2D texUnit, in int type, in int index, in vec2 uv, inout vec4 color){

    // Read from the texture
    vec4 tex = texture(texUnit,uv);
   
    if (type == REPLACE){
        color = tex;

    } else if (type == MODULATE){
        color *= tex;

    } else if (type == DECAL){
        vec3 temp = mix(color.rgb, tex.rgb, tex.a);
        color = vec4(temp, color.a);

    }else if (type == BLEND) {
        vec3 temp = mix(color.rgb, gl_TextureEnvColor[index].rgb, tex.rgb);
        color = vec4(temp, color.a * tex.a);

    }else if (type == ADD){
        color.rgb += tex.rgb;
        color.a   *= tex.a;
        color = clamp(color, 0.0, 1.0);

	} else if (type == SMOOTH_ADD) {
		color = (color + tex) - (color * tex);

	} else if (type == SIGNED_ADD){
		color += (tex - 0.5);

	}else if (type == DIVIDE){
		color /= tex;

	}else if (type == SUBSTRACT) {
		color -= tex;

	}else {
        color = clamp(tex * color, 0.0, 1.0);
    }
}