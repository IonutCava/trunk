-- Vertex

varying vec3 normals;
varying vec3 position;
varying vec4 texCoord[2];
varying mat4 TBN;

void main( void ){

	gl_Position =  gl_ModelViewProjectionMatrix * gl_Vertex;

	texCoord[0] = gl_MultiTexCoord0;
	position = vec3(transpose(gl_ModelViewMatrix) * gl_Vertex);
	normals = normalize(gl_NormalMatrix * gl_Normal);

	vec3 t = normalize(gl_NormalMatrix * gl_MultiTexCoord1.xyz);
	vec3 n = normalize(gl_NormalMatrix * gl_Normal);
	vec3 b = cross(n, t);

	
   TBN = mat4( t.x, b.x, n.x, 0,
               t.y, b.y, n.y, 0,
               t.z, b.z, n.z, 0,
               0, 0, 0, 1 );

} 

-- Fragment.NoTexture

varying vec3         normals;
varying vec3         position;
uniform mat4         material;

void main( void ){

	gl_FragData[0] = material[1]; //diffuse
    gl_FragData[1] = vec4(position,1);
    gl_FragData[2] = vec4(normals,1);
}

-- Fragment.Texture

varying vec3  normals;
varying vec3  position;
varying vec4  texCoord[2];

uniform sampler2D texDiffuse0;

void main( void ){
  vec4 color = texture2D(texDiffuse0,texCoord[0].st);
   if(color.a < 0.2) discard;
   gl_FragData[0] = color;
   gl_FragData[0].a = 0;
   gl_FragData[1] = vec4(position,1);
   gl_FragData[2] = vec4(normals,1);
}

-- Fragment.Bump

varying vec4       position;
varying vec4       texCoord[2];
varying mat4       TBN;
uniform sampler2D  texDiffuse0;
uniform sampler2D  texBump;

void main( void ){

   vec4 color = texture2D(texDiffuse0,texCoord[0].st);
   if(color.a < 0.2) discard;
   gl_FragData[0] = color;
   gl_FragData[1] = vec4(position.xyz,1);
   gl_FragData[2] = (texture2D(texBump,texCoord[0]) * 2 -
                     vec4(1,1,1,0)) * TBN;
}

-- Fragment.Impostor

varying vec4		position;
varying vec4		normals;
varying mat4		TBN;
uniform mat4        material;

void main( void )
{
	gl_FragData[0]		= material[1];
	gl_FragData[1]		= vec4(0,0,0,gl_FragData[0].a);
	gl_FragData[2]		= vec4(0,0,0,0);
}