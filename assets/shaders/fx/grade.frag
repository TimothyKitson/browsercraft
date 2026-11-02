#version 330 core
#include "common.glsl"

in vec2 vUv;
uniform sampler2D uScene;
uniform sampler2D uBloom;
uniform float uExposure;
uniform float uTime;
uniform float uRain;
uniform int uUnderwater;
uniform int uDimension;

out vec4 FragColor;

vec3 filmic(vec3 x)
{
    vec3 curve = 1.0 - exp(-x * 1.25);
    vec3 shoulder = x / (x + 0.9) * 1.9;
    return mix(curve, min(shoulder, vec3(1.0)), smoothstep(0.6, 1.0, curve) * 0.25);
}

void main()
{
    vec3 scene = texture(uScene, vUv).rgb;
    vec3 bloom = texture(uBloom, vUv).rgb;

    float bloomStrength = 0.55 + (uUnderwater == 1 ? 0.6 : 0.0) + uRain * 0.3;
    vec3 colour = scene + bloom * bloomStrength;
    colour *= uExposure;

    colour = filmic(colour);

    float grey = luma(colour);
    float vibrance = 0.22 * (1.0 - smoothstep(0.0, 0.6, max(max(colour.r, colour.g), colour.b) - min(min(colour.r, colour.g), colour.b)));
    colour = mix(vec3(grey), colour, 1.12 + vibrance);
    colour = max(colour, vec3(0.0));

    colour = pow(colour, vec3(1.0 / 2.2));

    vec2 centre = vUv - 0.5;
    float vignette = 1.0 - dot(centre, centre) * 0.42;
    colour *= vignette;

    colour += (ditherNoise(gl_FragCoord.xy) - 0.5) / 255.0;
    FragColor = vec4(clamp(colour, 0.0, 1.0), 1.0);
}
