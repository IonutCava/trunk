-- Vertex

#include "vertexDefault.vert"

void main(void)
{
	computeData();
}

-- Fragment

//----------------------------------------------------------------------------------
// File:   view_fragment.glsl
// Author: Rouslan Dimitrov
// Email:  sdkfeedback@nvidia.com
// Copyright (c) NVIDIA Corporation. All rights reserved.
//----------------------------------------------------------------------------------
in vec2 _texCoord;
out vec4 _colorOut;

uniform sampler2D tex;

void main()
{
	_colorOut = texture(tex, _texCoord);
	_colorOut.a = 0;
}

-- Fragment.Layered

//----------------------------------------------------------------------------------
// File:   view_fragment.glsl
// Author: Rouslan Dimitrov
// Email:  sdkfeedback@nvidia.com
// Copyright (c) NVIDIA Corporation. All rights reserved.
//----------------------------------------------------------------------------------
in vec2 _texCoord;
out vec4 _colorOut;

uniform sampler2DArray tex;
uniform int layer;
uniform vec2 dvd_zPlanes;

void main()
{
	float linearDepth = (2 * dvd_zPlanes.x) / (dvd_zPlanes.y + dvd_zPlanes.x - texture(tex, vec3(_texCoord, layer)).x * (dvd_zPlanes.y - dvd_zPlanes.x));
	_colorOut = vec4(linearDepth, linearDepth, linearDepth, 0.0);
}