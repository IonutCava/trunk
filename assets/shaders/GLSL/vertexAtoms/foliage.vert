uniform vec3  scale;
uniform float grassScale;
uniform float dvd_time;
uniform vec2 windDirection;
uniform float windSpeed;
uniform float lod_metric;

void computeFoliageMovementTree(inout vec4 vertex) {

	float move_speed = (float(int(_vertexM.y*_vertexM.z) % 50)/50.0 + 0.5);
	float timeTree = dvd_time * 0.001 * windSpeed * move_speed; //to seconds
	float amplituted = pow(vertex.y, 2.0);
	vertex.x += 0.01 * amplituted * cos(timeTree + _vertexM.x) *windDirection.x;
	vertex.z += 0.05 *scale.y* amplituted * cos(timeTree + _vertexM.z) *windDirection.y; ///wd.y is actually wd.z in code
}

void computeFoliageMovementGrass(in vec3 normal, in vec3 normalMV, inout vec4 vertex) {
	if(normal.y > 0.0 ) {
		float timeGrass = windSpeed * dvd_time * 0.001; //to seconds
		normalMV = -normalMV;
		float cosX = cos(vertex.x);
		float sinX = sin(vertex.x);
		float halfScale = 0.5*grassScale;
		vertex.x += (halfScale*cos(timeGrass) * cosX * sinX)*windDirection.x;
		vertex.z += (halfScale*sin(timeGrass) * cosX * sinX)*windDirection.y;
	}
}