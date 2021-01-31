-- Vertex.NoClouds

#include "vbInputData.vert"

void main(void){
    computeData(fetchInputData());

    VAR._vertexW.xyz += dvd_cameraPosition.xyz;
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

-- Fragment.NoClouds

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


--Vertex.Clouds

#define NEED_SCENE_DATA
#include "vbInputData.vert"
#include "sceneData.cmn"

const float rayleigh = 2.0;
const float turbidity = 10.0;
const float mieCoefficient = 0.005;

out vec3 vSunDirection;
out float vSunfade;
out vec3 vBetaR;
out vec3 vBetaM;
out float vSunE;
out vec3 vSunColor;
out vec3 vAmbient;

/*
Atmospheric scattering based off of: https://www.shadertoy.com/view/XtBXDz
Author: valentingalea
*/

// scattering coefficients at sea level (m)
const vec3 betaR = vec3(5.5e-6, 13.0e-6, 22.4e-6); // Rayleigh 
const vec3 betaM = vec3(21e-6); // Mie

// scale height (m)
// thickness of the atmosphere if its density were uniform
const float hR = 7994.0; // Rayleigh
const float hM = 1200.0; // Mie

const float earth_radius = 6360e3; // (m)
const float atmosphere_radius = 6420e3; // (m)

const float sun_power = 30.0;

//These numbers can be lowered to acheive faster speed, but they are good enough here
const int num_samples = 16;
const int num_samples_light = 8;

struct ray_t
{
    vec3 origin;
    vec3 direction;
};

bool isect_sphere(const in ray_t ray, inout float t0, inout float t1) {
    const vec3 rc = vec3(0.f) - ray.origin;
    const float radius2 = atmosphere_radius * atmosphere_radius;
    const float tca = dot(rc, ray.direction);
    const float d2 = dot(rc, rc) - tca * tca;
    if (d2 > radius2) {
        return false;
    }

    const float thc = sqrt(radius2 - d2);
    t0 = tca - thc;
    t1 = tca + thc;

    return true;
}

#define rayleigh_phase_func(MU) (3.f * (1.f + MU * MU) / (16.f * M_PI))

float HG(float costheta, float g) {
    const float k = 0.0795774715459f;
    return k * (1.f - g * g) / (pow(1.f + g * g - 2.f * g * costheta, 1.5f));
}

bool get_sun_light(const in ray_t ray, inout float optical_depthR, inout float optical_depthM) {
    float t0 = 0.f;
    float t1 = 0.f;
    isect_sphere(ray, t0, t1);

    float march_pos = 0.f;
    const float march_step = t1 / float(num_samples_light);

    for (int i = 0; i < num_samples_light; i++) {
        vec3 s = ray.origin + ray.direction * (march_pos + 0.5f * march_step);
        float height = length(s) - earth_radius;
        if (height < 0.f) {
            return false;
        }

        optical_depthR += exp(-height / hR) * march_step;
        optical_depthM += exp(-height / hM) * march_step;

        march_pos += march_step;
    }

    return true;
}

vec3 get_incident_light(const in ray_t ray) {
    // "pierce" the atmosphere with the viewing ray
    float t0 = 0.f;
    float t1 = 0.f;
    if (!isect_sphere(ray, t0, t1)) {
        return vec3(0.f);
    }

    const float march_step = t1 / float(num_samples);

    const vec3 sun_dir = normalize(-dvd_sunDirection.xyz);
    // cosine of angle between view and light directions
    float mu = dot(ray.direction, sun_dir);

    // Rayleigh and Mie phase functions
    float phaseR = rayleigh_phase_func(mu);
    float phaseM = HG(mu, 0.96);//0.76 more proper but this looks nice

    // optical depth (or "average density")
    // represents the accumulated extinction coefficients
    // along the path, multiplied by the length of that path
    float optical_depthR = 0.;
    float optical_depthM = 0.;

    vec3 sumR = vec3(0);
    vec3 sumM = vec3(0);
    float march_pos = 0.;

    for (int i = 0; i < num_samples; i++) {
        vec3 s = ray.origin + ray.direction * (march_pos + 0.5 * march_step);
        float height = length(s) - earth_radius;

        // integrate the height scale
        float hr = exp(-height / hR) * march_step;
        float hm = exp(-height / hM) * march_step;
        optical_depthR += hr;
        optical_depthM += hm;

        // gather the sunlight
        ray_t light_ray = ray_t(s, sun_dir);
        float optical_depth_lightR = 0.;
        float optical_depth_lightM = 0.;
        bool overground = get_sun_light(light_ray, optical_depth_lightR, optical_depth_lightM);

        if (overground) {
            vec3 tau = betaR * (optical_depthR + optical_depth_lightR) +
                       betaM * 1.1 * (optical_depthM + optical_depth_lightM);
            vec3 attenuation = exp(-tau);

            sumR += hr * attenuation;
            sumM += hm * attenuation;
        }

        march_pos += march_step;
    }

    return sun_power * (sumR * phaseR * betaR +
                        sumM * phaseM * betaM);
}

/*
Mostly Preetham model stuff here
*/
const vec3 up = vec3(0.0, 1.0, 0.0);

// constants for atmospheric scattering
const float e = 2.71828182845904523536028747135266249775724709369995957;
const float pi = 3.141592653589793238462643383279502884197169;

// wavelength of used primaries, according to preetham
const vec3 lambda = vec3(680E-9, 550E-9, 450E-9);
// this pre-calcuation replaces older TotalRayleigh(vec3 lambda) function:
// (8.0 * pow(pi, 3.0) * pow(pow(n, 2.0) - 1.0, 2.0) * (6.0 + 3.0 * pn)) / (3.0 * N * pow(lambda, vec3(4.0)) * (6.0 - 7.0 * pn))
const vec3 totalRayleigh = vec3(5.804542996261093E-6, 1.3562911419845635E-5, 3.0265902468824876E-5);

// mie stuff
// K coefficient for the primaries
const float v = 4.0;
const vec3 K = vec3(0.686, 0.678, 0.666);
// MieConst = pi * pow( ( 2.0 * pi ) / lambda, vec3( v - 2.0 ) ) * K
const vec3 MieConst = vec3(1.8399918514433978E14, 2.7798023919660528E14, 4.0790479543861094E14);

// earth shadow hack
// cutoffAngle = pi / 1.95;
const float cutoffAngle = 1.6110731556870734;
const float steepness = 1.5;
const float EE = 1000.0;

float sunIntensity(float zenithAngleCos) {
    zenithAngleCos = clamp(zenithAngleCos, -1.f, 1.f);
    return EE * max(0.f, 1.f - pow(e, -((cutoffAngle - acos(zenithAngleCos)) / steepness)));
}

#define totalMie(T) (0.434f * ((0.2f * T) * 10E-18) * MieConst)

void main() {
    computeData(fetchInputData());

    VAR._vertexW.xyz += dvd_cameraPosition.xyz;
    VAR._vertexWV = dvd_ViewMatrix * VAR._vertexW;
    gl_Position = dvd_ProjectionMatrix * VAR._vertexWV;
    gl_Position.z = gl_Position.w - SKY_OFFSET;

    vSunDirection = normalize(-dvd_sunDirection.xyz);

    vSunE = sunIntensity(dot(vSunDirection, up));

    vSunfade = 1.f - saturate(1.f - exp((-dvd_sunDirection.y / 450000.0f)));

    float rayleighCoefficient = rayleigh - (1.0 * (1.0 - vSunfade));
    // extinction (absorbtion + out scattering)
    // rayleigh coefficients
    vBetaR = totalRayleigh * rayleighCoefficient;
    // mie coefficients
    vBetaM = totalMie(turbidity) * mieCoefficient;

    ray_t ray = ray_t(vec3(0.0, earth_radius + 1.0, 0.0), normalize(vSunDirection + vec3(0.01, 0.01, 0.0)));
    vSunColor = get_incident_light(ray);

    ray = ray_t(vec3(0.0, earth_radius + 1.0, 0.0), normalize(vec3(0.4, 0.1, 0.0)));
    vAmbient = get_incident_light(ray);
}

--Fragment.Clouds

//TODO update cloud shaping
//TODO fix up weather texture
//TODO add more flexibility to parameters
    // rain/coverage
    // earth radius
    // sky color //maybe LUT // or just vec3 for coefficients
//TODO add 2d cloud layer on top


layout(early_fragment_tests) in;

layout(binding = TEXTURE_UNIT0) uniform samplerCube texSkyDay;
layout(binding = TEXTURE_UNIT1) uniform samplerCube texSkyNight;

layout(binding = TEXTURE_HEIGHT) uniform sampler2D weather;
layout(binding = TEXTURE_OPACITY) uniform sampler2D curl;
layout(binding = TEXTURE_OMR) uniform sampler3D worl;
layout(binding = TEXTURE_NORMALMAP) uniform sampler3D perlworl;

/*uniform vec4 dvd_atmosphereData[3];
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
*/
int check = 1;
float downscale = 1;

//preetham variables
in vec3 vSunDirection;
in float vSunfade;
in vec3 vBetaR;
in vec3 vBetaM;
in float vSunE;
in vec3 vAmbient;
in vec3 vSunColor;

out vec4 color;

//I like the look of the small sky, but could be tweaked however
const float g_radius = 200000.0; //ground radius
const float sky_b_radius = 201000.0;//bottom of cloud layer
const float sky_t_radius = 202300.0;//top of cloud layer
const float c_radius = 6008400.0; //2d noise layer

const float cwhiteScale = 1.1575370919881305;//precomputed 1/U2Tone(40)

/*
        SHARED FUNCTIONS
*/

vec3 U2Tone(const vec3 x) {
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;

    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}


float HG(float costheta, float g) {
    const float k = 0.0795774715459;
    return k * (1.0 - g * g) / (pow(1.0 + g * g - 2.0 * g * costheta, 1.5));
}


/*
        stars from a different project
        */

vec3 moonpos = vec3(0.3, 0.3, 0.0);


float SimplexPolkaDot3D(vec3 P, float density)	//	the maximal dimness of a dot ( 0.0->1.0   0.0 = all dots bright,  1.0 = maximum variation )
{
    //	calculate the simplex vector and index math
    vec3 Pi;
    vec3 Pi_1;
    vec3 Pi_2;
    vec4 v1234_x;
    vec4 v1234_y;
    vec4 v1234_z;

    vec3 Pn = P;
    //	simplex math constants
    float SKEWFACTOR = 1.0 / 3.0;
    float UNSKEWFACTOR = 1.0 / 6.0;
    float SIMPLEX_CORNER_POS = 0.5;
    float SIMPLEX_PYRAMID_HEIGHT = 0.70710678118654752440084436210485;	// sqrt( 0.5 )	height of simplex pyramid.

    Pn *= SIMPLEX_PYRAMID_HEIGHT;		// scale space so we can have an approx feature size of 1.0  ( optional )

    //	Find the vectors to the corners of our simplex pyramid
    Pi = floor(Pn + vec3(dot(Pn, vec3(SKEWFACTOR))));
    vec3 x0 = Pn - Pi + vec3(dot(Pi, vec3(UNSKEWFACTOR)));
    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = vec3(1.0) - g;
    Pi_1 = min(g.xyz, l.zxy);
    Pi_2 = max(g.xyz, l.zxy);
    vec3 x1 = x0 - Pi_1 + vec3(UNSKEWFACTOR);
    vec3 x2 = x0 - Pi_2 + vec3(SKEWFACTOR);
    vec3 x3 = x0 - vec3(SIMPLEX_CORNER_POS);

    //	pack them into a parallel-friendly arrangement
    v1234_x = vec4(x0.x, x1.x, x2.x, x3.x);
    v1234_y = vec4(x0.y, x1.y, x2.y, x3.y);
    v1234_z = vec4(x0.z, x1.z, x2.z, x3.z);

    vec3 gridcell = Pi;
    vec3 v1_mask = Pi_1;
    vec3 v2_mask = Pi_2;

    vec2 OFFSET = vec2(50.0, 161.0);
    float DOMAIN = 69.0;
    float SOMELARGEFLOAT = 6351.29681;
    float ZINC = 487.500388;

    //	truncate the domain
    gridcell.xyz = gridcell.xyz - floor(gridcell.xyz * (1.0 / DOMAIN)) * DOMAIN;
    vec3 gridcell_inc1 = step(gridcell, vec3(DOMAIN - 1.5)) * (gridcell + vec3(1.0));

    //	compute x*x*y*y for the 4 corners
    vec4 Pp = vec4(gridcell.xy, gridcell_inc1.xy) + vec4(OFFSET.xy, OFFSET.xy);
    Pp *= Pp;
    vec4 V1xy_V2xy = mix(vec4(Pp.xy, Pp.xy), vec4(Pp.zw, Pp.zw), vec4(v1_mask.xy, v2_mask.xy));		//	apply mask for v1 and v2
    Pp = vec4(Pp.x, V1xy_V2xy.x, V1xy_V2xy.z, Pp.z) * vec4(Pp.y, V1xy_V2xy.y, V1xy_V2xy.w, Pp.w);

    vec2 V1z_V2z = vec2(gridcell_inc1.z);
    if (v1_mask.z < 0.5) {
        V1z_V2z.x = gridcell.z;
    }
    if (v2_mask.z < 0.5) {
        V1z_V2z.y = gridcell.z;
    }
    vec4 temp = vec4(SOMELARGEFLOAT) + vec4(gridcell.z, V1z_V2z.x, V1z_V2z.y, gridcell_inc1.z) * ZINC;
    vec4 mod_vals = vec4(1.0) / (temp);

    //	compute the final hash
    vec4 hash = fract(Pp * mod_vals);


    //	apply user controls
    float INV_SIMPLEX_TRI_HALF_EDGELEN = 2.3094010767585030580365951220078;	// scale to a 0.0->1.0 range.  2.0 / sqrt( 0.75 )
    float radius = INV_SIMPLEX_TRI_HALF_EDGELEN;///(1.15-density);
    v1234_x *= radius;
    v1234_y *= radius;
    v1234_z *= radius;

    //	return a smooth falloff from the closest point.  ( we use a f(x)=(1.0-x*x)^3 falloff )
    vec4 point_distance = max(vec4(0.0), vec4(1.0) - (v1234_x * v1234_x + v1234_y * v1234_y + v1234_z * v1234_z));
    point_distance = point_distance * point_distance * point_distance;
    vec4 b = (vec4(density) - hash) * (1.0 / density);
    b = max(vec4(0.0), b);
    b = min(vec4(1.0), b);
    b = pow(b, vec4(1.0 / density));
    return dot(b, point_distance);
}

vec3 stars(vec3 ndir) {

    vec3 COLOR = vec3(0.0);
    float star = SimplexPolkaDot3D(ndir * 100.0, 0.15) + SimplexPolkaDot3D(ndir * 150.0, 0.25) * 0.7;
    vec3 col = vec3(0.05, 0.07, 0.1);
    COLOR.rgb = col + max(0.0, (star - smoothstep(0.2, 0.95, 0.5 - 0.5 * ndir.y)));
    COLOR.rgb += vec3(0.05, 0.07, 0.1) * (1.0 - smoothstep(-0.1, 0.45, ndir.y));
    //need to add a little bit of atmospheric effects, both for when the moon is high
    //and for the end when color comes into the sky
    //For moon halo
    //COLOR.rgb += smoothstep(0.9, 1.0, dot(ndir, normalize(moonpos)));
    //float d = length(ndir-normalize(moonpos));
    //COLOR.rgb += 0.8*exp(-4.0*d)*vec3(1.1, 1.0, 0.8);
    //COLOR.rgb += 0.2*exp(-2.0*d);

    return COLOR;
}

/*
//This implementation of the preetham model is a modified: https://github.com/mrdoob/three.js/blob/master/examples/js/objects/Sky.js
//written by: zz85 / https://github.com/zz85
        */

const float mieDirectionalG = 0.8;

// constants for atmospheric scattering
const float pi = 3.141592653589793238462643383279502884197169;

// optical length at zenith for molecules
const float rayleighZenithLength = 8.4E3;
const float mieZenithLength = 1.25E3;
const vec3 up = vec3(0.0, 1.0, 0.0);
// 66 arc seconds -> degrees, and the cosine of that
const float sunAngularDiameterCos = 0.999956676946448443553574619906976478926848692873900859324;

// 3.0 / ( 16.0 * pi )
const float THREE_OVER_SIXTEENPI = 0.05968310365946075;

const float whiteScale = 1.0748724675633854; // 1.0 / Uncharted2Tonemap(1000.0)

vec3 preetham(const vec3 vWorldPosition) {
    // optical length
    // cutoff angle at 90 to avoid singularity in next formula.
    float zenithAngle = acos(max(0.0, dot(up, normalize(vWorldPosition))));
    float inv = 1.0 / (cos(zenithAngle) + 0.15 * pow(93.885 - ((zenithAngle * 180.0) / pi), -1.253));
    float sR = rayleighZenithLength * inv;
    float sM = mieZenithLength * inv;

    // combined extinction factor
    vec3 Fex = exp(-(vBetaR * sR + vBetaM * sM));

    // in scattering
    float cosTheta = dot(normalize(vWorldPosition), vSunDirection);

    float rPhase = THREE_OVER_SIXTEENPI * (1.0 + pow(cosTheta * 0.5 + 0.5, 2.0));
    vec3 betaRTheta = vBetaR * rPhase;

    float mPhase = HG(cosTheta, mieDirectionalG);
    vec3 betaMTheta = vBetaM * mPhase;

    vec3 Lin = pow(vSunE * ((betaRTheta + betaMTheta) / (vBetaR + vBetaM)) * (1.0 - Fex), vec3(1.5));
    Lin *= mix(vec3(1.0), pow(vSunE * ((betaRTheta + betaMTheta) / (vBetaR + vBetaM)) * Fex, vec3(1.0 / 2.0)), clamp(pow(1.0 - dot(up, vSunDirection), 5.0), 0.0, 1.0));

    vec3 L0 = vec3(0.5) * Fex;

    // composition + solar disc
    float sundisk = smoothstep(sunAngularDiameterCos, sunAngularDiameterCos + 0.00002, cosTheta);
    L0 += (vSunE * 19000.0 * Fex) * sundisk;

    vec3 texColor = (Lin + L0) * 0.04 + vec3(0.0, 0.0003, 0.00075);

    vec3 curr = U2Tone(texColor);
    vec3 color = curr * whiteScale;

    vec3 retColor = pow(color, vec3(1.0 / (1.2 + (1.2 * vSunfade))));

    return retColor;
}
/*
        ===============================================================
        end of atmospheric scattering
*/

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

vec4 mixGradients(const float cloudType) {

    const vec4 STRATUS_GRADIENT = vec4(0.02f, 0.05f, 0.09f, 0.11f);
    const vec4 STRATOCUMULUS_GRADIENT = vec4(0.02f, 0.2f, 0.48f, 0.625f);
    const vec4 CUMULUS_GRADIENT = vec4(0.01f, 0.0625f, 0.78f, 1.0f); // these fractions would need to be altered if cumulonimbus are added to the same pass
    float stratus = 1.0f - clamp(cloudType * 2.0f, 0.0, 1.0);
    float stratocumulus = 1.0f - abs(cloudType - 0.5f) * 2.0f;
    float cumulus = clamp(cloudType - 0.5f, 0.0, 1.0) * 2.0f;
    return STRATUS_GRADIENT * stratus + STRATOCUMULUS_GRADIENT * stratocumulus + CUMULUS_GRADIENT * cumulus;
}

float densityHeightGradient(const float heightFrac, const float cloudType) {
    vec4 cloudGradient = mixGradients(cloudType);
    return smoothstep(cloudGradient.x, cloudGradient.y, heightFrac) - smoothstep(cloudGradient.z, cloudGradient.w, heightFrac);
}

float intersectSphere(const vec3 pos, const vec3 dir, const float r) {
    float a = dot(dir, dir);
    float b = 2.0 * dot(dir, pos);
    float c = dot(pos, pos) - (r * r);
    float d = sqrt((b * b) - 4.0 * a * c);
    float p = -b - d;
    float p2 = -b + d;
    return max(p, p2) / (2.0 * a);
}

// Utility function that maps a value from one range to another. 
float remap(const float originalValue, const float originalMin, const float originalMax, const float newMin, const float newMax) {
    return newMin + (((originalValue - originalMin) / (originalMax - originalMin)) * (newMax - newMin));
}

float density(vec3 p, vec3 weather, const bool hq, const float LOD) {
    float time = MSToSeconds(dvd_time);

    p.x += time * 20.0;
    float height_fraction = GetHeightFractionForPoint(length(p));
    vec4 n = textureLod(perlworl, p * 0.0003, LOD);
    float fbm = n.g * 0.625 + n.b * 0.25 + n.a * 0.125;
    float g = densityHeightGradient(height_fraction, 0.5);
    float base_cloud = remap(n.r, -(1.0 - fbm), 1.0, 0.0, 1.0);
    float cloud_coverage = smoothstep(0.6, 1.3, weather.x);
    base_cloud = remap(base_cloud * g, 1.0 - cloud_coverage, 1.0, 0.0, 1.0);
    base_cloud *= cloud_coverage;
    if (hq) {
        vec2 whisp = texture(curl, p.xy * 0.0003).xy;
        p.xy += whisp * 400.0 * (1.0 - height_fraction);
        vec3 hn = texture(worl, p * 0.004, LOD - 2.0).xyz;
        float hfbm = hn.r * 0.625 + hn.g * 0.25 + hn.b * 0.125;
        hfbm = mix(hfbm, 1.0 - hfbm, clamp(height_fraction * 3.0, 0.0, 1.0));
        base_cloud = remap(base_cloud, hfbm * 0.2, 1.0, 0.0, 1.0);
    }
    return clamp(base_cloud, 0.0, 1.0);
}

vec4 march(const vec3 pos, const vec3 end, vec3 dir, const int depth) {
    float T = 1.0;
    float alpha = 0.0;
    vec3 p = pos;
    float ss = length(dir);
    const float t_dist = sky_t_radius - sky_b_radius;
    float lss = t_dist / float(depth);
    vec3 ldir = vSunDirection * ss;
    vec3 L = vec3(0.0);
    int count = 0;
    float t = 1.0;
    float costheta = dot(normalize(ldir), normalize(dir));
    float phase = max(max(HG(costheta, 0.6), HG(costheta, (0.99 - 1.3 * normalize(ldir).y))), HG(costheta, -0.3));
    for (int i = 0; i < depth; i++) {
        p += dir;
        float height_fraction = GetHeightFractionForPoint(length(p));
        const float weather_scale = 0.00008;
        vec3 weather_sample = texture(weather, p.xz * weather_scale).xyz;
        t = density(p, weather_sample, true, 0.0);
        const float ldt = 0.5;
        float dt = exp(-ldt * t * ss);
        T *= dt;
        vec3 lp = p;
        const float ld = 0.5;
        float lt = 1.0;
        float ncd = 0.0;
        float cd = 0.0;
        if (t > 0.0) { //calculate lighting, but only when we are in a non-zero density point
            for (int j = 0; j < 6; j++) {
                lp += (ldir + (RANDOM_VECTORS[j] * float(j + 1)) * lss);
                vec3 lweather = texture(weather, lp.xz * weather_scale).xyz;
                lt = density(lp, lweather, false, float(j));
                cd += lt;
                ncd += (lt * (1.0 - (cd * (1.0 / (lss * 6.0)))));
            }
            lp += ldir * 12.0;
            vec3 lweather = texture(weather, lp.xz * weather_scale).xyz;
            lt = density(lp, lweather, false, 5.0);
            cd += lt;
            ncd += (lt * (1.0 - (cd * (1.0 / (lss * 18.0)))));

            float beers = max(exp(-ld * ncd * lss), exp(-ld * 0.25 * ncd * lss) * 0.7);
            float powshug = 1.0 - exp(-ld * ncd * lss * 2.0);

            vec3 ambient = 5.0 * vAmbient * mix(0.15, 1.0, height_fraction);
            vec3 sunC = pow(vSunColor, vec3(0.75));
            L += (ambient + sunC * beers * powshug * 2.0 * phase) * (t)*T * ss;
            alpha += (1.0 - dt) * (1.0 - alpha);
        }
    }
    return vec4(L, alpha);
}


void main() {
    vec2 shift = vec2(floor(float(check) / downscale), mod(float(check), downscale));
    //shift = vec2(0.0);
    vec2 uv = (gl_FragCoord.xy * downscale + shift.yx) / (dvd_screenDimensions);
    uv = uv - vec2(0.5);
    uv *= 2.0;
    uv.x *= dvd_aspectRatio;
    vec4 uvdir = (vec4(uv.xy, 1.0, 1.0));
    vec4 worldPos = (inverse((dvd_ViewProjectionMatrix)) * uvdir);
    vec3 dir = normalize(worldPos.xyz / worldPos.w);

    vec4 col = vec4(0.0);
    if (dir.y > 0.0) {

        vec3 camPos = vec3(0.0, g_radius, 0.0);
        vec3 start = camPos + dir * intersectSphere(camPos, dir, sky_b_radius);
        vec3 end = camPos + dir * intersectSphere(camPos, dir, sky_t_radius);
        const float t_dist = sky_t_radius - sky_b_radius;
        float shelldist = (length(end - start));
        vec4 volume;
        float steps = (mix(96.0, 54.0, dot(dir, vec3(0.0, 1.0, 0.0))));
        float dmod = smoothstep(0.0, 1.0, (shelldist / t_dist) / 14.0);
        float s_dist = mix(t_dist, t_dist * 4.0, dmod) / (steps);
        vec3 raystep = dir * s_dist;
        volume = march(start, end, raystep, int(steps));
        volume.xyz = U2Tone(volume.xyz) * cwhiteScale;
        volume.xyz = sqrt(volume.xyz);
        vec3 background = vec3(0.0);
        if (volume.a < 0.99) {
            background = preetham(dir);
            background = max(background, stars(dir));
        }

        col = vec4(background * (1.0 - volume.a) + volume.xyz * volume.a, 1.0);
        if (volume.a > 1.0) { col = vec4(1.0, 0.0, 0.0, 1.0); }
    } else {
        col = vec4(vec3(0.4), 1.0);
    }
    color = col;
}