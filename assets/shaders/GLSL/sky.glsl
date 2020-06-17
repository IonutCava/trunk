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

uniform vec3 dvd_sunDirection;
uniform vec3 dvd_nightSkyColour;
uniform ivec2 dvd_useSkyboxes;
uniform uint dvd_raySteps = 16;

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

#include "utility.frag"
#include "output.frag"

#define PI 3.141592
#define iSteps dvd_raySteps
#define jSteps int(dvd_raySteps * 0.5f)

vec3 ray_dir_from_uv(vec2 uv) {
    vec3 dir;

    float x = sin(PI * uv.y);
    dir.y = cos(PI * uv.y);

    dir.x = x * sin(2.0 * PI * (0.5 - uv.x));
    dir.z = x * cos(2.0 * PI * (0.5 - uv.x));

    return dir;
}

vec2 uv_from_ray_dir(vec3 dir) {
    vec2 uv;

    uv.y = acos(dir.y) / PI;

    dir.y = 0.0;
    dir = normalize(dir);
    uv.x = acos(dir.z) / (2.0 * PI);
    if (dir.x < 0.0) {
        uv.x = 1.0 - uv.x;
    }
    uv.x = 0.5 - uv.x;
    if (uv.x < 0.0) {
        uv.x += 1.0;
    }

    return uv;
}

//ref: https://github.com/wwwtyro/glsl-atmosphere
vec2 rsi(vec3 r0, vec3 rd, float sr) {
    // ray-sphere intersection that assumes
    // the sphere is centered at the origin.
    // No intersection when result.x > result.y
    float a = dot(rd, rd);
    float b = 2.0 * dot(rd, r0);
    float c = dot(r0, r0) - (sr * sr);
    float d = (b * b) - 4.0 * a * c;
    if (d < 0.0) return vec2(1e5, -1e5);
    return vec2(
        (-b - sqrt(d)) / (2.0 * a),
        (-b + sqrt(d)) / (2.0 * a)
    );
}

vec3 atmosphere(vec3 r, vec3 r0, vec3 pSun, float iSun, float rPlanet, float rAtmos, vec3 kRlh, float kMie, float shRlh, float shMie, float g) {
    // Normalize the sun and view directions.
    pSun = normalize(pSun);
    r = normalize(r);

    // Calculate the step size of the primary ray.
    vec2 p = rsi(r0, r, rAtmos);
    if (p.x > p.y) return vec3(0, 0, 0);
    p.y = min(p.y, rsi(r0, r, rPlanet).x);
    float iStepSize = (p.y - p.x) / float(iSteps);

    // Initialize the primary ray time.
    float iTime = 0.0;

    // Initialize accumulators for Rayleigh and Mie scattering.
    vec3 totalRlh = vec3(0, 0, 0);
    vec3 totalMie = vec3(0, 0, 0);

    // Initialize optical depth accumulators for the primary ray.
    float iOdRlh = 0.0;
    float iOdMie = 0.0;

    // Calculate the Rayleigh and Mie phases.
    float mu = dot(r, pSun);
    float mumu = mu * mu;
    float gg = g * g;
    float pRlh = 3.0 / (16.0 * PI) * (1.0 + mumu);
    float pMie = 3.0 / (8.0 * PI) * ((1.0 - gg) * (mumu + 1.0)) / (pow(1.0 + gg - 2.0 * mu * g, 1.5) * (2.0 + gg));

    // Sample the primary ray.
    for (int i = 0; i < iSteps; i++) {

        // Calculate the primary ray sample position.
        vec3 iPos = r0 + r * (iTime + iStepSize * 0.5);

        // Calculate the height of the sample.
        float iHeight = length(iPos) - rPlanet;

        // Calculate the optical depth of the Rayleigh and Mie scattering for this step.
        float odStepRlh = exp(-iHeight / shRlh) * iStepSize;
        float odStepMie = exp(-iHeight / shMie) * iStepSize;

        // Accumulate optical depth.
        iOdRlh += odStepRlh;
        iOdMie += odStepMie;

        // Calculate the step size of the secondary ray.
        float jStepSize = rsi(iPos, pSun, rAtmos).y / float(jSteps);

        // Initialize the secondary ray time.
        float jTime = 0.0;

        // Initialize optical depth accumulators for the secondary ray.
        float jOdRlh = 0.0;
        float jOdMie = 0.0;

        // Sample the secondary ray.
        for (int j = 0; j < jSteps; j++) {

            // Calculate the secondary ray sample position.
            vec3 jPos = iPos + pSun * (jTime + jStepSize * 0.5);

            // Calculate the height of the sample.
            float jHeight = length(jPos) - rPlanet;

            // Accumulate the optical depth.
            jOdRlh += exp(-jHeight / shRlh) * jStepSize;
            jOdMie += exp(-jHeight / shMie) * jStepSize;

            // Increment the secondary ray time.
            jTime += jStepSize;
        }

        // Calculate attenuation.
        vec3 attn = exp(-(kMie * (iOdMie + jOdMie) + kRlh * (iOdRlh + jOdRlh)));

        // Accumulate scattering.
        totalRlh += odStepRlh * attn;
        totalMie += odStepMie * attn;

        // Increment the primary ray time.
        iTime += iStepSize;

    }

    float spot = smoothstep(0.0f, 1000.0f, pMie) * dvd_sunIntensity;

    // Calculate and return the final color.
    return iSun * (spot * totalMie + pRlh * kRlh * totalRlh + pMie * kMie * totalMie);
}

vec3 ACESFilm(vec3 x) {
    const float tA = 2.51f;
    const float tB = 0.03f;
    const float tC = 2.43f;
    const float tD = 0.59f;
    const float tE = 0.14f;
    return clamp((x * (tA * x + tB)) / (x * (tC * x + tD) + tE), 0.0f, 1.0f);
}

vec3 dayColour(in vec3 rayDirection) {
    const float luminanceThreshold = 0.75f;

    vec3 atmoColour =
        atmosphere(
                    rayDirection,
                    vec3(0, dvd_planetRadius + dvd_rayOriginOffset, 0),
                    -dvd_sunDirection,
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
            const vec3 sky = (texture(texSkyDay, vec4(rayDirection, 0)).rgb);
            atmoColour = mix((atmoColour + sky) - (atmoColour * sky),
                             atmoColour,
                             luminance(atmoColour.rgb));
    }

    return ACESFilm(atmoColour);
}

vec3 nightColour(in vec3 rayDirection) {
    if (dvd_useNightSkybox) {
        return texture(texSkyNight, vec4(rayDirection, 0)).rgb;
    }

    return dvd_nightSkyColour;
}

void main() {
    const vec3 dir = VAR._vertexW.xyz - dvd_cameraPosition.xyz;
    const float lerpValue = saturate(2.95f * (dvd_sunDirection.y + 0.15f));
    writeOutput(vec4(mix(dayColour(dir), nightColour(dir), lerpValue), 1.0f));
}

--Fragment.PrePass

#include "prePass.frag"

void main() {
#if defined(HAS_PRE_PASS_DATA)
    NodeData data = dvd_Matrices[DATA_IDX];
    prepareData(data);

    writeOutput(data, 
                VAR._texCoord,
                VAR._normalWV,
                1.0f,
                1.0f);
#endif //HAS_PRE_PASS_DATA
}