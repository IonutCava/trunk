-- Vertex

#include "vbInputData.vert"

void main(void){
    computeData(fetchInputData());

    VAR._vertexW += vec4(dvd_cameraPosition.xyz, 0.0f);
    //setClipPlanes();

    VAR._vertexWV = dvd_ViewMatrix * VAR._vertexW;
    VAR._vertexWVP = dvd_ProjectionMatrix * VAR._vertexWV;
    gl_Position = VAR._vertexWVP;
    gl_Position.z = gl_Position.w - SKY_OFFSET;
}

-- Fragment.Display

layout(early_fragment_tests) in;

layout(binding = TEXTURE_UNIT0) uniform samplerCubeArray texSkyDay;
layout(binding = TEXTURE_UNIT1) uniform samplerCubeArray texSkyNight;

uniform vec3 dvd_nightSkyColour;
uniform ivec2 dvd_useSkyboxes;

uniform vec4 dvd_atmosphereData[3];

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

vec3 dayColour(in vec3 rayDirection) {
    vec3 atmoColour =
        atmosphere(
                    rayDirection,
                    vec3(0, dvd_planetRadius + dvd_rayOriginOffset, 0),
                    -dvd_sunDirection.xyz,
                    dvd_sunIntensity,
                    dvd_planetRadius,
                    dvd_planetRadius + dvd_atmosphereOffset,
                    dvd_RayleighCoeff,
                    dvd_MieCoeff,
                    dvd_RayleighScale,
                    dvd_MieScaleHeight,
                    dvd_MieScatterDir
                );

    if (dvd_useDaySkybox) {
        const vec3 sky = texture(texSkyDay, vec4(rayDirection, 0)).rgb;
        atmoColour = mix((atmoColour + sky) - (atmoColour * sky),
                          atmoColour,
                          luminance(atmoColour.rgb));
    }

    return ACESFilm(atmoColour);
}

vec3 nightColour(in vec3 rayDirection) {
    return dvd_useNightSkybox ? texture(texSkyNight, vec4(rayDirection, 0.f)).rgb
                              : dvd_nightSkyColour;
}

void main() {
    const vec3 dir = VAR._vertexW.xyz - dvd_cameraPosition.xyz;

    // Guess work based on what "look right"
    const float lerpValue = saturate(2.95f * (dvd_sunDirection.y + 0.15f));

    writeOutput(vec4(mix(dayColour(dir),
                         nightColour(dir),
                         lerpValue),
                     1.0f)
                );
}