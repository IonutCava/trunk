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

void main()
{
	float fLayer = layer;
	vec3 tex_coord = vec3(_texCoord, fLayer);
	_colorOut = texture(tex, tex_coord);
	_colorOut.a = 0;
}