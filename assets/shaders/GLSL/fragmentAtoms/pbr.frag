// AMAZING RESOURCE : http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_moving_frostbite_to_pbr.pdf
// Reference: https://github.com/urho3d/Urho3D/blob/master/bin/CoreData/Shaders/GLSL/PBR.glsl
// Following BRDF methods are based upon research Frostbite EA
//[Lagrade et al. 2014, "Moving Frostbite to Physically Based Rendering"]

#include "IBL.frag"


//-------------------- VISIBILITY ---------------------------------------------------
// Smith GGX corrected Visibility
//   NdotL        = the dot product of the normal and direction to the light
//   NdotV        = the dot product of the normal and the camera view direction
//   roughness    = the roughness of the pixel
float SmithGGXSchlickVisibility(float NdotL, float NdotV, float roughness)
{
    const float rough2 = roughness * roughness;
    const float lambdaV = NdotL  * sqrt((-NdotV * rough2 + NdotV) * NdotV + rough2);
    const float lambdaL = NdotV  * sqrt((-NdotL * rough2 + NdotL) * NdotL + rough2);

    return 0.5f / (lambdaV + lambdaL);
}

// Neumann Visibility
//   NdotV        = the dot product of the normal and the camera view direction
//   NdotL        = the dot product of the normal and direction to the light
float NeumannVisibility(float NdotV, float NdotL)
{
    return NdotL * NdotV / max(1e-7, max(NdotL, NdotV));
}
//----------------------------------------------------------------------------------

//-------------------- DISTRIBUTION-------------------------------------------------
// Blinn Distribution
//   NdotH        = the dot product of the normal and the half vector
//   roughness    = the roughness of the pixel
float BlinnPhongDistribution(in float NdotH, in float roughness)
{
    const float specPower = max((2.0f / (roughness * roughness)) - 2.0f, 1e-4); // Calculate specular power from roughness
    return pow(saturate(NdotH), specPower);
}

// Beckmann Distribution
//   NdotH        = the dot product of the normal and the half vector
//   roughness    = the roughness of the pixel
float BeckmannDistribution(in float NdotH, in float roughness)
{
    const float rough2 = SQUARED(roughness);
    const float NdotH2 = SQUARED(NdotH);

    const float roughnessA = 1.0f / (4.0f * rough2 * pow(NdotH, 4.0f));
    const float roughnessB = NdotH2 - 1.0f;
    const float roughnessC = rough2 * NdotH2;
    return roughnessA * exp(roughnessB / roughnessC);
}

// GGX Distribution
//   NdotH        = the dot product of the normal and the half vector
//   roughness    = the roughness of the pixel
float GGXDistribution(float NdotH, float roughness)
{
    const float rough2 = SQUARED(roughness);
    const float tmp = (NdotH * rough2 - NdotH) * NdotH + 1.0f;
    return rough2 / (tmp * tmp);
}
//----------------------------------------------------------------------------------


//-------------------- DIFFUSE -----------------------------------------------------
// Lambertian Diffuse
//   diffuseColor = the rgb color value of the pixel
//   NdotV        = the normal dot with the camera view direction
//   roughness    = the roughness of the pixel
vec3 LambertianDiffuse(vec3 diffuseColor, float NdotV, float roughness)
{
    return diffuseColor * INV_M_PI;
}

// Custom Lambertian Diffuse
//   diffuseColor = the rgb color value of the pixel
//   roughness    = the roughness of the pixel
//   NdotV        = the normal dot with the camera view direction
//   NdotL        = the normal dot with the light direction
//   VdotH        = the camera view direction dot with the half vector
vec3 CustomLambertianDiffuse(vec3 diffuseColor, float NdotV, float roughness)
{
    return diffuseColor * INV_M_PI * pow(NdotV, 0.5f + 0.3f * roughness);
}

// Burley Diffuse
//   diffuseColor = the rgb color value of the pixel
//   roughness    = the roughness of the pixel
//   NdotV        = the normal dot with the camera view direction
//   NdotL        = the normal dot with the light direction
//   VdotH        = the camera view direction dot with the half vector
vec3 BurleyDiffuse(vec3 diffuseColor, float roughness, float NdotV, float NdotL, float VdotH)
{
    const float energyBias = mix(roughness, 0.0f, 0.5f);
    const float energyFactor = mix(roughness, 1.0f, 1.0f / 1.51f);
    const float fd90 = energyBias + 2.0f * VdotH * VdotH * roughness;
    const float f0 = 1.0f;
    const float lightScatter = f0 + (fd90 - f0) * pow(1.0f - NdotL, 5.0f);
    const float viewScatter = f0 + (fd90 - f0) * pow(1.0f - NdotV, 5.0f);

    return diffuseColor * lightScatter * viewScatter * energyFactor;
}
//----------------------------------------------------------------------------------

//-------------------- FRESNEL -----------------------------------------------------
// Schlick Fresnel
//   specular  = the rgb specular color value of the pixel
//   VdotH     = the dot product of the camera view direction and the half vector 
vec3 SchlickFresnel(in vec3 specular, in float VdotH) {
    //return mix(specular, vec3(1.0f), pow(1.0f - VdotH, 5.0f));
    return specular + (vec3(1.0f) - specular) * pow(1.0f - VdotH, 5.0f);
}

// Schlick Gaussian Fresnel
//   specular  = the rgb specular color value of the pixel
//   VdotH     = the dot product of the camera view direction and the half vector 
vec3 SchlickGaussianFresnel(in vec3 specular, in float VdotH) {
    float sphericalGaussian = pow(2.0f, (-5.55473f * VdotH - 6.98316f) * VdotH);
    return specular + (vec3(1.0f) - specular) * sphericalGaussian;
}

// Schlick Gaussian Fresnel
//   specular  = the rgb specular color value of the pixel
//   VdotH     = the dot product of the camera view direction and the half vector 
vec3 SchlickFresnelCustom(in vec3 specular, in float LdotH) {
    const float ior = 0.25f;
    const float airIor = 1.000277f;
    float f0 = (ior - airIor) / (ior + airIor);
    const float max_ior = 2.5;
    f0 = clamp(f0 * f0, 0.0, (max_ior - airIor) / (max_ior + airIor));
    return specular * (f0 + (1 - f0) * pow(2, (-5.55473 * LdotH - 6.98316) * LdotH));
}

//----------------------------------------------------------------------------------


//-------------------- GETTERS - -----------------------------------------------------
//Get Fresnel
    //specular  = the rgb specular color value of the pixel
    //VdotH     = the dot product of the camera view direction and the half vector 
vec3 Fresnel(in vec3 specular, in float VdotH, in float LdotH) {
#if defined(PBR_SHADING)
    return SchlickGaussianFresnel(specular, LdotH);
#else
    return SchlickFresnel(specular, VdotH);
#endif
}

// Get Distribution
    // NdotH        = the dot product of the normal and the half vector
    // roughness    = the roughness of the pixel
float Distribution(float NdotH, float roughness) {
#if defined(PBR_SHADING)
    return GGXDistribution(NdotH, roughness);
#else
    return BlinnPhongDistribution(NdotH, roughness);
#endif
}

//Get Diffuse
    // diffuseColor = the rgb color value of the pixel
    // roughness    = the roughness of the pixel
    // NdotV        = the normal dot with the camera view direction
    // NdotL        = the normal dot with the light direction
    // VdotH        = the camera view direction dot with the half vector
vec3 Diffuse(vec3 diffuseColor, float roughness, float NdotV, float NdotL, float VdotH) {
#if defined(PBR_SHADING)
    return BurleyDiffuse(diffuseColor, roughness, NdotV, NdotL, VdotH);
#else
    return LambertianDiffuse(diffuseColor, NdotV, roughness);
    //return CustomLambertianDiffuse(diffuseColor, NdotV, roughness);
#endif
}

// Get Visibility
    // NdotL        = the dot product of the normal and direction to the light
    // NdotV        = the dot product of the normal and the camera view direction
    // roughness    = the roughness of the pixel
float Visibility(float NdotL, float NdotV, float roughness) {
#if defined(PBR_SHADING)
    return NeumannVisibility(NdotV, NdotL);
#else
    return SmithGGXSchlickVisibility(NdotL, NdotV, roughness);
#endif
}

//Return the PBR BRDF value
    // lightDir  = the vector to the light
    // lightVec  = normalised lightDir
    // toCamera  = view direction
    // normal    = surface normal of the pixel
    // ndl       = dot(normal,lightVec) [M_EPSILON,1.0f]
vec3 GetBRDF(in vec3 lightDir,
             in vec3 lightVec,
             in vec3 toCamera,
             in vec3 normal,
             in vec3 diffColour,
             in vec3 specColour,
             in float ndl,
             in float roughness,
             out float specularFactorOut)
{
#if defined(PBR_SHADING) || defined(USE_SHADING_PHONG) || defined (USE_SHADING_BLINN_PHONG)
    // Vector data
    const vec3  Hn  = normalize(toCamera + lightDir);
    const float vdh = clamp((dot(toCamera, Hn)), M_EPSILON, 1.0f);
    const float ndh = clamp((dot(normal, Hn)), M_EPSILON, 1.0f);
    const float ndv = clamp((dot(normal, toCamera)), M_EPSILON, 1.0f);
    const float ldh = clamp((dot(lightVec, Hn)), M_EPSILON, 1.0);

    // Material data
    const vec3 diffuseFactor = Diffuse(diffColour, roughness, ndv, ndl, vdh) * ndl;
    const vec3 fresnelTerm   = Fresnel(specColour, vdh, ldh);
    const float distTerm     = Distribution(ndh, roughness);
    const float visTerm      = Visibility(ndl, ndv, roughness);

    const vec3 specularFactor = distTerm * visTerm * fresnelTerm * ndl * INV_M_PI;
    specularFactorOut = length(specularFactor);

    return diffuseFactor + specularFactor;
#elif defined(USE_SHADING_TOON)
    const float intensity = dot(lightVec, normal);
    specularFactorOut = intensity;
    vec3 color2 = vec3(1.0f);
    if (intensity > 0.95f)      color2 = vec3(1.0f);
    else if (intensity > 0.75f) color2 = vec3(0.8f);
    else if (intensity > 0.50f) color2 = vec3(0.6f);
    else if (intensity > 0.25f) color2 = vec3(0.4f);
    else                        color2 = vec3(0.2f);
    return color2 * diffColour;
#else
    specularFactorOut = 0.0f;
    return vec3(0.6f, 1.0f, 0.7f); //obvious lime-green
#endif //USE_SHADING_TOON
}