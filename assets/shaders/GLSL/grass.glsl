-- Vertex

#define HAS_TRANSPARENCY
#define NO_VELOCITY
#define NEED_SCENE_DATA
#include "vbInputData.vert"
#include "lightingDefaults.vert"
#include "vegetationData.cmn"
#include "sceneData.cmn"

layout(location = 0) flat out uvec2 _layerAndLoD;
layout(location = 1) flat out uint _instanceID;
layout(location = 2) out float _alphaFactor;

#define GRASS_DISPLACEMENT_DISTANCE 50.0f
#define GRASS_DISPLACEMENT_MAGNITUDE 0.5f

void smallScaleMotion(inout vec4 vertexW, in float heightExtent, in float time) {
    const float disp = 0.064f * sin(2.65f * (vertexW.x + vertexW.y + vertexW.z + (time * 3)));
    vertexW.xz += heightExtent * vec2(disp, disp);
}

void largetScaleMotion(inout vec4 vertexW, in vec4 vertex, in float heightExtent, in float time) {
    const float X = (2.f * sin(1.f * (vertex.x + vertex.y + vertex.z + time))) + 1.0f;
    const float Y = 0.f;
    const float Z = (1.f * sin(2.f * (vertex.x + vertex.y + vertex.z + time))) + 0.5f;

    vertexW.xyz += heightExtent * vec3(X * dvd_windDetails.x, Y, Z * dvd_windDetails.z);
    smallScaleMotion(vertexW, heightExtent, time * 1.25f);
}

vec3 rotate_vertex_position(vec3 v, vec4 q) {
    return v + 2.f * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

void main() {
    const NodeTransformData nodeData = fetchInputData();

    VegetationData data = GrassData(gl_InstanceID);
    _instanceID = gl_InstanceID;

#if defined(USE_CULL_DISTANCE)
    if (data.data.z > 2.5f) {
        gl_CullDistance[0] = -1.0f;
    }
    float scale = data.positionAndScale.w;
#else
    float scale = data.data.z > 2.5f ? 0.0f : data.positionAndScale.w * saturate(data.data.z);
#endif

#if defined(PRE_PASS)
    _alphaFactor = 1.0f;
#else
    _alphaFactor = saturate(data.data.z);
#endif

    _layerAndLoD.x = uint(data.data.x);
    _layerAndLoD.y = uint(data.data.y);

    const float timeGrass = dvd_windDetails.w * MSToSeconds(dvd_time) * 0.5f;
    const bool animate = _layerAndLoD.y < 2u && dvd_Vertex.y > 0.5f;

    dvd_Vertex.xyz *= scale;
    dvd_Vertex.xyz = rotate_vertex_position(dvd_Vertex.xyz, data.orientationQuad);
    //dvd_Vertex.y += 0.25f;
    VAR._vertexW = dvd_Vertex + vec4(data.positionAndScale.xyz, 0.0f);

    if (animate) {
        const float displacement = (GRASS_DISPLACEMENT_MAGNITUDE * (1.f - smoothstep(GRASS_DISPLACEMENT_DISTANCE, GRASS_DISPLACEMENT_DISTANCE * 1.5f, data.data.w)));
        const vec2 toCamera = normalize(dvd_cameraPosition.xz - VAR._vertexW.xz);
        VAR._vertexW.xz -= toCamera * displacement;
        //VAR._vertexW.xz += (mat3(dvd_InverseViewMatrix) * vec3(0.f, 0.f, -1.f)).xz * displacement;

        largetScaleMotion(VAR._vertexW, dvd_Vertex, dvd_Vertex.y * 0.075f, timeGrass);
    }

#if !defined(SHADOW_PASS)
    setClipPlanes();
#endif

    mat3 normalMatrixWV = mat3(dvd_ViewMatrix) * dvd_NormalMatrixW(nodeData);

    VAR._vertexWV = dvd_ViewMatrix * VAR._vertexW;
    if (_layerAndLoD.y < 2u) {
        VAR._normalWV = normalize(normalMatrixWV * rotate_vertex_position(dvd_Normal, data.orientationQuad));
    }

    computeLightVectors(nodeData);

    gl_Position = dvd_ProjectionMatrix * VAR._vertexWV;
}

-- Fragment.Colour

layout(early_fragment_tests) in;

#define NO_VELOCITY
#define SAMPLER_UNIT0_IS_ARRAY
#define USE_SHADING_BLINN_PHONG
#define HAS_TRANSPARENCY
#define SKIP_DOUBLE_SIDED_NORMALS
#define NO_IBL
#define MAX_SHADOW_MAP_LOD 1
//#define DEBUG_LODS

#include "BRDF.frag"

#include "utility.frag"
#include "output.frag"

layout(location = 0) flat in uvec2 _layerAndLoD;
layout(location = 1) flat in uint _instanceID;
layout(location = 2) in float _alphaFactor;

void main (void){
    NodeMaterialData data = dvd_Materials[MATERIAL_IDX];

    const uint LoD = _layerAndLoD.y;
    vec4 albedo = texture(texDiffuse0, vec3(VAR._texCoord, _layerAndLoD.x));

    if (_instanceID % 3 == 0) {
        albedo.rgb = overlayVec(albedo.rgb, vec3(0.9f, 0.85f, 0.55f));
    } else if (_instanceID % 4 == 0) {
        albedo.rgb = overlayVec(albedo.rgb, vec3(0.75f, 0.65f, 0.45f));
    }

#if defined(DEBUG_LODS)
    if (LoD == 0) {
        albedo.rgb = vec3(1.f, 0.f, 0.f);
    } else if (LoD == 1) {
        albedo.rgb = vec3(0.f, 1.f, 0.f);
    } else if (LoD == 2) {
        albedo.rgb = vec3(0.f, 0.f, 1.f);
    } else if (LoD == 3) {
        albedo.rgb = vec3(1.f, 0.f, 1.f);
    } else {
        albedo.rgb = vec3(0.f, 1.f, 1.f);
    }
#endif //DEBUG_LODS

    albedo.a = min(albedo.a, _alphaFactor);

    const vec3 normalWV = getNormalWV(VAR._texCoord);
    vec3 MetalnessRoughnessProbeID = vec3(0.f, 1.f, 0.f);
    vec3 SpecularColourOut = vec3(0.f);
    const vec4 colour = getPixelColour(LoD, albedo, data, normalWV, VAR._texCoord, SpecularColourOut, MetalnessRoughnessProbeID);
    writeScreenColour(colour, normalWV, SpecularColourOut, MetalnessRoughnessProbeID);
}

--Fragment.PrePass

// This is needed so that the inner "output" discards don't take place
// We still manually alpha-discard in main
#define SAMPLER_UNIT0_IS_ARRAY
#define USE_ALPHA_DISCARD
#define NO_VELOCITY
#include "prePass.frag"

layout(location = 0) flat in uvec2 _layerAndLoD;
layout(location = 1) flat in uint _instanceID;
layout(location = 2) in float _alphaFactor;

void main() {
    writeGBuffer(texture(texDiffuse0, vec3(VAR._texCoord, _layerAndLoD.x)).a * _alphaFactor);
}

--Fragment.Shadow.VSM

layout(location = 0) flat in uvec2 _layerAndLoD;
layout(location = 1) flat in uint _instanceID;
layout(location = 2) in float _alphaFactor;

layout(binding = TEXTURE_UNIT0) uniform sampler2DArray texDiffuse0;

#include "vsm.frag"
out vec2 _colourOut;

void main(void) {
    const float albedoAlpha = texture(texDiffuse0, vec3(VAR._texCoord, _layerAndLoD.x)).a;
    if (albedoAlpha * _alphaFactor < INV_Z_TEST_SIGMA) {
        discard;
    }

    _colourOut = computeMoments();
}