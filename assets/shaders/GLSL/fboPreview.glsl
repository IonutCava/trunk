-- Vertex

//----------------------------------------------------------------------------------
// File:   view_vertex.glsl
// Author: Rouslan Dimitrov
// Email:  sdkfeedback@nvidia.com
// Copyright (c) NVIDIA Corporation. All rights reserved.
//----------------------------------------------------------------------------------
varying vec2 _texCoord;

void main()
{
	_texCoord = vec4(vec4(0.5)*gl_Vertex + vec4(0.5)).xy;
	gl_Position = gl_Vertex;
}
-- Fragment

//----------------------------------------------------------------------------------
// File:   view_fragment.glsl
// Author: Rouslan Dimitrov
// Email:  sdkfeedback@nvidia.com
// Copyright (c) NVIDIA Corporation. All rights reserved.
//----------------------------------------------------------------------------------
varying vec2 _texCoord;

uniform sampler2D tex;

void main()
{
	gl_FragColor = texture(tex, _texCoord);
}

-- Fragment.Layered

//----------------------------------------------------------------------------------
// File:   view_fragment.glsl
// Author: Rouslan Dimitrov
// Email:  sdkfeedback@nvidia.com
// Copyright (c) NVIDIA Corporation. All rights reserved.
//----------------------------------------------------------------------------------
varying vec2 _texCoord;

uniform sampler2DArray tex;
uniform int layer;

void main()
{
	float fLayer = layer;
	vec4 tex_coord = vec4(_texCoord.x, _texCoord.y,fLayer, 1.0);
	gl_FragColor = texture(tex, tex_coord.xyz);
}