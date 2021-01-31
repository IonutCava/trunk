-- Vertex

#include "vbInputData.vert"

void main(void){
    computeData(fetchInputData());

    VAR._vertexW += vec4(dvd_cameraPosition.xyz, 0.0f);
    //setClipPlanes();

    VAR._vertexWV = dvd_ViewMatrix * VAR._vertexW;
    gl_Position = dvd_ProjectionMatrix * VAR._vertexWV;
    gl_Position.z = gl_Position.w - SKY_OFFSET;
}

-- Fragment.PassThrough

layout(early_fragment_tests) in;
#include "output.frag"

void main() {
    writeOutput(vec4(0.0f, 0.75f, 0.0f, 1.0f));
}

-- Fragment.Display

layout(early_fragment_tests) in;

layout(binding = TEXTURE_UNIT0) uniform samplerCube texSkyDay;
layout(binding = TEXTURE_UNIT1) uniform samplerCube texSkyNight;

uniform vec4 dvd_atmosphereData[3];
uniform vec3 dvd_nightSkyColour;
uniform ivec2 dvd_useSkyboxes;

#define dvd_sunIntensity  dvd_atmosphereData[0].x
#define dvd_planetRadius  dvd_atmosphereData[0].y
#define dvd_atmosphereOffset  dvd_atmosphereData[0].z
#define dvd_MieCoeff  dvd_atmosphereData[0].w

#define dvd_RayleighCoeff dvd_atmosphereData[1].xyz
#define dvd_RayleighScale dvd_atmosphereData[1].w

#define dvd_MieScaleHeight dvd_atmosphereData[2].x
#define dvd_MieScatterDir dvd_atmosphereData[2].y
#define dvd_rayOriginOffset dvd_atmosphereData[2].z

#define dvd_useDaySkybox (dvd_useSkyboxes.x == 1)
#define dvd_useNightSkybox (dvd_useSkyboxes.y == 1)

#include "environment.frag"
#include "output.frag"

vec3 ACESFilm(vec3 x) {
    const float tA = 2.51f;
    const float tB = 0.03f;
    const float tC = 2.43f;
    const float tD = 0.59f;
    const float tE = 0.14f;
    return saturate((x * (tA * x + tB)) / (x * (tC * x + tD) + tE));
}

//ToDo: https://github.com/clayjohn/realtime_clouds/blob/master/assets/shaders/sky.frag
//I like the look of the small sky, but could be tweaked however
const float g_radius = 200000.0; //ground radius
const float sky_b_radius = 201000.0;//bottom of cloud layer
const float sky_t_radius = 202300.0;//top of cloud layer
const float c_radius = 6008400.0; //2d noise layer
const float cwhiteScale = 1.1575370919881305;//precomputed 1/U2Tone(40)

const vec3 RANDOM_VECTORS[6] = vec3[6]
(
    vec3(0.38051305f, 0.92453449f, -0.02111345f),
    vec3(-0.50625799f, -0.03590792f, -0.86163418f),
    vec3(-0.32509218f, -0.94557439f, 0.01428793f),
    vec3(0.09026238f, -0.27376545f, 0.95755165f),
    vec3(0.28128598f, 0.42443639f, -0.86065785f),
    vec3(-0.16852403f, 0.14748697f, 0.97460106f)
);
// fractional value for sample position in the cloud layer
float GetHeightFractionForPoint(float inPosition) { // get global fractional position in cloud zone
    float height_fraction = (inPosition - sky_b_radius) / (sky_t_radius - sky_b_radius);
    return clamp(height_fraction, 0.0, 1.0);
}

vec3 atmosphereColour(in vec3 rayDirection) {
    return atmosphere(rayDirection,
                      vec3(0, dvd_planetRadius + dvd_rayOriginOffset, 0),
                      -dvd_sunDirection.xyz,
                      dvd_sunIntensity,
                      dvd_planetRadius,
                      dvd_planetRadius + dvd_atmosphereOffset,
                      dvd_RayleighCoeff,
                      dvd_MieCoeff,
                      dvd_RayleighScale,
                      dvd_MieScaleHeight,
                      dvd_MieScatterDir);
}

vec3 dayColour(in vec3 rayDirection) {
    vec3 atmoColour = atmosphereColour(rayDirection);

    if (dvd_useDaySkybox) {
        const vec3 sky = texture(texSkyDay, rayDirection).rgb;

        atmoColour = mix((atmoColour + sky) - (atmoColour * sky),
                          atmoColour,
                          Luminance(atmoColour.rgb));
    }

    return ACESFilm(atmoColour);
}

vec3 nightColour(in vec3 rayDirection) {
    return dvd_useNightSkybox ? texture(texSkyNight, rayDirection).rgb
                              : dvd_nightSkyColour;
}

vec3 albedoOnly(in vec3 rayDirection, in float lerpValue) {
    return mix(dvd_useDaySkybox ? texture(texSkyDay, rayDirection).rgb : vec3(1.0f),
               nightColour(rayDirection),
               lerpValue);
}

vec3 finalColour(in vec3 rayDirection, in float lerpValue) {
    switch (dvd_materialDebugFlag) {
        case DEBUG_ALBEDO:        return albedoOnly(rayDirection, lerpValue);
        case DEBUG_DEPTH:         return vec3(1.0f);
        case DEBUG_LIGHTING:      return ACESFilm(atmosphereColour(rayDirection));
        case DEBUG_SPECULAR:      return vec3(0.0f);
        case DEBUG_UV:            return vec3(fract(rayDirection));
        case DEBUG_SSAO:          return vec3(1.0f);
        case DEBUG_EMISSIVE:      return ACESFilm(atmosphereColour(rayDirection));
        case DEBUG_ROUGHNESS:
        case DEBUG_METALLIC:
        case DEBUG_NORMALS:
        case DEBUG_TANGENTS:
        case DEBUG_BITANGENTS:    return vec3(0.0f);
        case DEBUG_SHADOW_MAPS:
        case DEBUG_CSM_SPLITS:    return vec3(1.0f);
        case DEBUG_LIGHT_HEATMAP:
        case DEBUG_DEPTH_CLUSTERS:
        case DEBUG_REFLECTIONS:
        case DEBUG_REFLECTIVITY:
        case DEBUG_MATERIAL_IDS:  return vec3(0.0f);
    }

    return mix(dayColour(rayDirection), nightColour(rayDirection), lerpValue);
}

void main() {
    const vec3 dir = VAR._vertexW.xyz - dvd_cameraPosition.xyz;

    // Guess work based on what "look right"
    const float lerpValue = saturate(2.95f * (dvd_sunDirection.y + 0.15f));

    writeOutput(vec4(finalColour(dir, lerpValue), 1.0f));
}