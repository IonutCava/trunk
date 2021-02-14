#ifndef _PBR_FRAG_
#define _PBR_FRAG_

// AMAZING RESOURCE : http://www.frostbite.com/wp-content/uploads/2014/11/course_notes_moving_frostbite_to_pbr.pdf
// Reference: https://github.com/urho3d/Urho3D/blob/master/bin/CoreData/Shaders/GLSL/PBR.glsl
// Following BRDF methods are based upon research Frostbite EA
//[Lagrade et al. 2014, "Moving Frostbite to Physically Based Rendering"]


vec3 SchlickFresnel(in vec3 specular, in float VdotH);

float SchlickFresnel(in float u) {
    const float m = 1.f - u;
    const float m2 = m * m;
    return m2 * m2 * m; // pow(m,5)
}

//-------------------- VISIBILITY ---------------------------------------------------
// Smith GGX corrected Visibility
//   NdotL        = the dot product of the normal and direction to the light
//   NdotV        = the dot product of the normal and the camera view direction
//   roughness    = the roughness of the pixel
float SmithGGXSchlickVisibility(in float NdotL, in float NdotV, in float roughness)
{
    const float rough2 = roughness * roughness;
    const float lambdaV = NdotL  * sqrt((-NdotV * rough2 + NdotV) * NdotV + rough2);
    const float lambdaL = NdotV  * sqrt((-NdotL * rough2 + NdotL) * NdotL + rough2);

    return 0.5f / (lambdaV + lambdaL);
}

// Neumann Visibility
//   NdotV        = the dot product of the normal and the camera view direction
//   NdotL        = the dot product of the normal and direction to the light
float NeumannVisibility(in float NdotV, in float NdotL)
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
float GGXDistribution(in float NdotH, in float roughness)
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
vec3 LambertianDiffuse(in vec3 diffuseColor, in float NdotV, in float roughness)
{
    return diffuseColor * INV_M_PI;
}

// Custom Lambertian Diffuse
//   diffuseColor = the rgb color value of the pixel
//   roughness    = the roughness of the pixel
//   NdotV        = the normal dot with the camera view direction
//   NdotL        = the normal dot with the light direction
//   VdotH        = the camera view direction dot with the half vector
vec3 CustomLambertianDiffuse(in vec3 diffuseColor, in float NdotV, in float roughness)
{
    return diffuseColor * INV_M_PI * pow(NdotV, 0.5f + 0.3f * roughness);
}

// Burley Diffuse
//   diffuseColor = the rgb color value of the pixel
//   roughness    = the roughness of the pixel
//   NdotV        = the normal dot with the camera view direction
//   NdotL        = the normal dot with the light direction
//   VdotH        = the camera view direction dot with the half vector
vec3 BurleyDiffuse(in vec3 diffuseColor, in float roughness, in float NdotV, in float NdotL, in float VdotH, in float LdotH)
{
#if 0
    const float energyBias = mix(roughness, 0.0f, 0.5f);
    const float energyFactor = mix(roughness, 1.f, 1.f / 1.51f);
    const float fd90 = energyBias + 2.f * VdotH * VdotH * roughness;
    const float f0 = 1.f;
    const float lightScatter = f0 + (fd90 - f0) * pow(1.f - NdotL, 5.f);
    const float viewScatter = f0 + (fd90 - f0) * pow(1.f - NdotV, 5.f);
    return diffuseColor * lightScatter * viewScatter * energyFactor;
#else
    const float INV_FD90 = 2.f * LdotH * LdotH * roughness - 0.5f;
    const float FdV = 1.f + INV_FD90 * SchlickFresnel(NdotV);
    const float FdL = 1.f + INV_FD90 * SchlickFresnel(NdotL);
    return diffuseColor * INV_M_PI * FdV * FdL * NdotL;
#endif
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
//   LdotH     = the dot product of the light direction and the half vector 
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
float Distribution(in float NdotH, in float roughness) {
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
vec3 Diffuse(in vec3 diffuseColor, in float roughness, in float NdotV, in float NdotL, in float VdotH, in float LdotH) {

#if defined(PBR_SHADING)
    return BurleyDiffuse(diffuseColor, roughness, NdotV, NdotL, VdotH, LdotH);
#else
    return LambertianDiffuse(diffuseColor, NdotV, roughness) * NdotL;
    //return CustomLambertianDiffuse(diffuseColor, NdotV, roughness) * NdotL;
#endif
}

// Get Visibility
    // NdotL        = the dot product of the normal and direction to the light
    // NdotV        = the dot product of the normal and the camera view direction
    // roughness    = the roughness of the pixel
float Visibility(in float NdotL, in float NdotV, in float roughness) {
#if defined(PBR_SHADING)
    return NeumannVisibility(NdotV, NdotL);
#else
    return SmithGGXSchlickVisibility(NdotL, NdotV, roughness);
#endif
}

//Return the PBR BRDF value
    // lightVec  = normalised lightDir
    // toCamera  = view direction
    // normal    = surface normal of the pixel
    // ndl       = dot(normal,lightVec) [M_EPSILON,1.0f]
vec3 GetBRDF(in vec3 L,
             in vec3 V,
             in vec3 N,
             in vec3 diffColour,
             in vec3 specColour,
             in float ndl,
             in float roughness)
{
#if defined(PBR_SHADING) || defined(USE_SHADING_PHONG) || defined (USE_SHADING_BLINN_PHONG)
    const vec3 H = normalize(V + L);

    const float vdh = clamp((dot(V, H)), M_EPSILON, 1.f);
    const float ldh = clamp((dot(L, H)), M_EPSILON, 1.f);
    const float ndh = clamp((dot(N, H)), M_EPSILON, 1.f);
    const float ndv = abs(dot(N, V)) + M_EPSILON;


    // Material data
    const vec3 diffuseFactor = Diffuse(diffColour, roughness, ndv, ndl, vdh, ldh);

    const vec3 fresnelTerm   = Fresnel(specColour, vdh, ldh);
    const float distTerm     = Distribution(ndh, roughness);
    const float visTerm      = Visibility(ndl, ndv, roughness);

    const vec3 specularFactor = distTerm * visTerm * fresnelTerm * ndl * INV_M_PI;

    return diffuseFactor + specularFactor;
#elif defined(USE_SHADING_TOON)
    const float intensity = dot(lightVec, normal);
    vec3 color2 = vec3(1.0f);
    if (intensity > 0.95f)      color2 = vec3(1.0f);
    else if (intensity > 0.75f) color2 = vec3(0.8f);
    else if (intensity > 0.50f) color2 = vec3(0.6f);
    else if (intensity > 0.25f) color2 = vec3(0.4f);
    else                        color2 = vec3(0.2f);
    return color2 * diffColour;
#else
    return vec3(0.6f, 1.0f, 0.7f); //obvious lime-green
#endif //USE_SHADING_TOON
}

#endif //_PBR_FRAG_
