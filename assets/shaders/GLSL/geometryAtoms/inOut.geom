#if defined(USE_GEOMETRY_TRIANGLE_INPUT)
layout(triangles) in;
#else if define(USE_GEOMETRY_LINE_INPUT)
layout(lines) in;
#else
layout(points) in;
#endif 
layout (triangle_strip, max_vertices=3) out;

in vec4  _vertexWV[];
in vec3  _normalWV[];
in vec3  _viewDirection[];
in vec3 _lightDirection[MAX_LIGHT_COUNT][]; //<Light direction
in vec4 _shadowCoord[MAX_SHADOW_CASTING_LIGHTS][];
in float _attenuation[MAX_LIGHT_COUNT][];

void main() {
#if defined(USE_GEOMETRY_TRIANGLE_INPUT)
  for(int i = 0; i < 3; i++) {
#else if define(USE_GEOMETRY_LINE_INPUT)
  for(int i = 0; i < 2; i++) {
#else
	int i = 0; {
#endif
    gl_Position = gl_in[i].gl_Position;
    EmitVertex();
  }
  EndPrimitive();
}