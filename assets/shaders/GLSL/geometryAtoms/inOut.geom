#if defined(USE_GEOMETRY_STRIP_INPUT)
layout(triangles) in;
#else
layout(triangle_strip) in;
#endif
layout (triangle_strip, max_vertices=3) out;
 
in vec4  _vertexMV[];
in vec3  _normalMV[];
in vec3  _viewDirection[];
in vec3 _lightDirection[MAX_LIGHT_COUNT][]; //<Light direction
in vec4 _shadowCoord[MAX_SHADOW_CASTING_LIGHTS][];
in float _attenuation[MAX_LIGHT_COUNT][];

void main() {
#if defined(USE_GEOMETRY_STRIP_INPUT)
  for(int i = 0; i < gl_in.length(); i++) {
#else
  for(int i = 0; i < 3; i++) {
#endif
    gl_Position = gl_in[i].gl_Position;
    EmitVertex();
  }
  EndPrimitive();
}