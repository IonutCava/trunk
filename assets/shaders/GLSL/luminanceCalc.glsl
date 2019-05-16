-- Fragment

#include "utility.frag"

out float _colourOut;

layout(binding = TEXTURE_UNIT0) uniform sampler2D texScreen;
layout(binding = TEXTURE_UNIT1) uniform sampler2D texPrevLuminance;

void main() {
    //Human eye:
    // - bright sunlight -> complete dark: 20-30 min
    // - complete dark -> bright sunlight: 5 min
    // - speed diff ~5
    vec3 screenColour = texture(texScreen, VAR._texCoord).rgb;
    float crtluminance = clamp(luminance(screenColour), 0.35, 0.75);
    float prevLuminance = clamp(exp(texture(texPrevLuminance, VAR._texCoord).r), 0.35f, 0.75f);
    // Slowly change luminance by mimicking human eye behaviour:
    // bright->dark 5 times faster than dark->bright
    float tau = mix(0.15f, 0.35f, prevLuminance > crtluminance);
    _colourOut = prevLuminance + (crtluminance - prevLuminance) * (1.0f - exp(-(dvd_deltaTime * 0.001f) * tau));
}