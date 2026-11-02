#version 330 core
#include "common.glsl"

in vec2 vUv;
uniform sampler2D uSource;
uniform vec2 uTexel;
uniform int uFirst;

out vec4 FragColor;

vec3 tap(vec2 offset)
{
    vec3 c = texture(uSource, vUv + offset * uTexel).rgb;
    if (uFirst == 1)
    {
        float l = luma(c);
        c *= (smoothstep(0.85, 2.6, l) + 0.04) / (1.0 + l * 0.25);
    }
    return c;
}

void main()
{
    vec3 centre = tap(vec2(0.0));
    vec3 ring = tap(vec2(-1.0, -1.0)) + tap(vec2(1.0, -1.0)) + tap(vec2(-1.0, 1.0)) + tap(vec2(1.0, 1.0));
    vec3 outer = tap(vec2(-2.0, 0.0)) + tap(vec2(2.0, 0.0)) + tap(vec2(0.0, -2.0)) + tap(vec2(0.0, 2.0));
    FragColor = vec4(centre * 0.25 + ring * 0.125 + outer * 0.0625, 1.0);
}
