// AMAZING RESOURCE : http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_moving_frostbite_to_pbr.pdf
// Reference: https://github.com/urho3d/Urho3D/blob/master/bin/CoreData/Shaders/GLSL/PBR.glsl
// Following BRDF methods are based upon research Frostbite EA
//[Lagrade et al. 2014, "Moving Frostbite to Physically Based Rendering"]

//Schlick Fresnel
//specular  = the rgb specular color value of the pixel
//VdotH     = the dot product of the camera view direction and the half vector 
vec3 SchlickFresnel(vec3 specular, float VdotH)
{
    return specular + (vec3(1.0, 1.0, 1.0) - specular) * pow(1.0 - VdotH, 5.0);
}

//Schlick Gaussian Fresnel
//specular  = the rgb specular color value of the pixel
//VdotH     = the dot product of the camera view direction and the half vector 
vec3 SchlickGaussianFresnel(in vec3 specular, in float VdotH)
{
    float sphericalGaussian = pow(2.0, (-5.55473 * VdotH - 6.98316) * VdotH);
    return specular + (vec3(1.0, 1.0, 1.0) - specular) * sphericalGaussian;
}

// Smith GGX corrected Visibility
// NdotL        = the dot product of the normal and direction to the light
// NdotV        = the dot product of the normal and the camera view direction
// roughness    = the roughness of the pixel
float SmithGGXSchlickVisibility(float NdotL, float NdotV, float roughness)
{
    float rough2 = roughness * roughness;
    float lambdaV = NdotL  * sqrt((-NdotV * rough2 + NdotV) * NdotV + rough2);
    float lambdaL = NdotV  * sqrt((-NdotL * rough2 + NdotL) * NdotL + rough2);

    return 0.5 / (lambdaV + lambdaL);
}

/// Schlick approximation of Smith GGX
///     nDotL: dot product of surface normal and light direction
///     nDotV: dot product of surface normal and view direction
///     roughness: surface roughness
float SchlickG1(in float factor, in float rough2)
{
    return 1.0 / (factor * (1.0 - rough2) + rough2);
}


float SchlickVisibility(float NdotL, float NdotV, float roughness)
{
    const float rough2 = roughness * roughness;
    return (SchlickG1(NdotL, rough2) * SchlickG1(NdotV, rough2)) * 0.25;
}

// Blinn Distribution
// NdotH        = the dot product of the normal and the half vector
// roughness    = the roughness of the pixel
float BlinnPhongDistribution(in float NdotH, in float roughness)
{
    float specPower = max((2.0 / (roughness * roughness)) - 2.0, 1e-4); // Calculate specular power from roughness
    return pow(clamp(NdotH, 0.0, 1.0), specPower);
}

// Beckmann Distribution
// NdotH        = the dot product of the normal and the half vector
// roughness    = the roughness of the pixel
float BeckmannDistribution(in float NdotH, in float roughness)
{
    float rough2 = roughness * roughness;
    float roughnessA = 1.0 / (4.0 * rough2 * pow(NdotH, 4.0));
    float roughnessB = NdotH * NdotH - 1.0;
    float roughnessC = rough2 * NdotH * NdotH;
    return roughnessA * exp(roughnessB / roughnessC);
}

// GGX Distribution
// NdotH        = the dot product of the normal and the half vector
// roughness    = the roughness of the pixel
float GGXDistribution(float NdotH, float roughness)
{
    float rough2 = roughness * roughness;
    float tmp = (NdotH * rough2 - NdotH) * NdotH + 1.0;
    return rough2 / (tmp * tmp);
}

// Lambertian Diffuse
// diffuseColor = the rgb color value of the pixel
// roughness    = the roughness of the pixel
// NdotV        = the normal dot with the camera view direction
// NdotL        = the normal dot with the light direction
// VdotH        = the camera view direction dot with the half vector
vec3 LambertianDiffuse(vec3 diffuseColor, float NdotL)
{
    return diffuseColor * NdotL;
}

// Burley Diffuse
// diffuseColor = the rgb color value of the pixel
// roughness    = the roughness of the pixel
// NdotV        = the normal dot with the camera view direction
// NdotL        = the normal dot with the light direction
// VdotH        = the camera view direction dot with the half vector
vec3 BurleyDiffuse(vec3 diffuseColor, float roughness, float NdotV, float NdotL, float VdotH)
{
    float energyBias = mix(roughness, 0.0, 0.5);
    float energyFactor = mix(roughness, 1.0, 1.0 / 1.51);
    float fd90 = energyBias + 2.0 * VdotH * VdotH * roughness;
    float f0 = 1.0;
    float lightScatter = f0 + (fd90 - f0) * pow(1.0f - NdotL, 5.0f);
    float viewScatter = f0 + (fd90 - f0) * pow(1.0f - NdotV, 5.0f);

    return diffuseColor * lightScatter * viewScatter * energyFactor;
}


// Get Distribution
// NdotH        = the dot product of the normal and the half vector
// roughness    = the roughness of the pixel
float Distribution(float NdotH, float roughness)
{
    return GGXDistribution(NdotH, roughness);
}

//Get Fresnel
//specular  = the rgb specular color value of the pixel
//VdotH     = the dot product of the camera view direction and the half vector 
vec3 Fresnel(vec3 specular, float VdotH)
{
    return SchlickFresnel(specular, VdotH);
}

// Get Visibility
// NdotL        = the dot product of the normal and direction to the light
// NdotV        = the dot product of the normal and the camera view direction
// roughness    = the roughness of the pixel
float Visibility(float NdotL, float NdotV, float roughness)
{
    return SmithGGXSchlickVisibility(NdotL, NdotV, roughness);
}

//Get Diffuse
// diffuseColor = the rgb color value of the pixel
// roughness    = the roughness of the pixel
// NdotV        = the normal dot with the camera view direction
// NdotL        = the normal dot with the light direction
// VdotH        = the camera view direction dot with the half vector
vec3 Diffuse(vec3 diffuseColor, float roughness, float NdotV, float NdotL, float VdotH)
{
    //return LambertianDiffuse(diffuseColor, NdotL);
    return BurleyDiffuse(diffuseColor, roughness, NdotV, NdotL, VdotH);
}

#define M_PI 3.14159265358979323846
#define M_EPSILON 0.0000001

vec4 PBR(in vec3 lightDirection, 
         in vec4 lightColourAndAtt,
         in vec4 specular,
         in vec4 albedoAndShadow,
         in vec3 normalWV,
         in vec3 viewDir)
{
    float roughness = specular.a;

    // direction is NOT normalized
    vec3 Hn = normalize(viewDir + lightDirection);
    float vdh = clamp((dot(viewDir, Hn)), M_EPSILON, 1.0);
    float ndh = clamp((dot(normalWV, Hn)), M_EPSILON, 1.0);
    float ndl = clamp((dot(normalWV, normalize(lightDirection))), M_EPSILON, 1.0);
    float ndv = clamp((dot(normalWV, viewDir)), M_EPSILON, 1.0);
    vec3 diffuseFactor = Diffuse(albedoAndShadow.rgb, roughness, ndv, ndl, vdh) * albedoAndShadow.a;

    vec3 fresnelTerm = Fresnel(specular.rgb, vdh);
    float distTerm = Distribution(ndh, roughness);
    float visTerm = Visibility(ndl, ndv, roughness);
    vec3 specularFactor = (fresnelTerm * distTerm * visTerm / M_PI) * albedoAndShadow.a;

    return vec4((lightColourAndAtt.rgb * (diffuseFactor + specularFactor) * lightColourAndAtt.a) / M_PI, length(specularFactor));
}