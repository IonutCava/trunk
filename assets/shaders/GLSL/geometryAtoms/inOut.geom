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
in vec3 _lightDirection[MAX_LIGHTS_PER_NODE][]; //<Light direction
in float _attenuation[MAX_LIGHTS_PER_NODE][];

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