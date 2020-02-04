#ifndef _VERTEX_DEFAULT_VERT_
#define _VERTEX_DEFAULT_VERT_

#include "nodeBufferedInput.cmn"

layout(location = ATTRIB_POSITION) in vec3 inVertexData;
layout(location = ATTRIB_TEXCOORD) in vec2 inTexCoordData;
layout(location = ATTRIB_NORMAL) in float inNormalData;
layout(location = ATTRIB_TANGENT) in float inTangentData;
layout(location = ATTRIB_COLOR) in vec4 inColourData;
layout(location = ATTRIB_BONE_WEIGHT) in vec4 inBoneWeightData;
layout(location = ATTRIB_BONE_INDICE) in uvec4 inBoneIndiceData;
layout(location = ATTRIB_WIDTH) in uint inLineWidthData;
layout(location = ATTRIB_GENERIC) in vec2 inGenericData;

#endif //_VERTEX_DEFAULT_VERT_