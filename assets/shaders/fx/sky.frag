#version 330 core
#include "common.glsl"

in vec2 vUv;

uniform mat4 uInvViewProj;
uniform vec3 uCameraPos;
uniform vec3 uSunDirection;
uniform vec3 uLightColour;
uniform vec3 uSkyAmbient;
uniform float uTime;
uniform float uRain;
uniform sampler2D uNoise;

out vec4 FragColor;

const float CLOUD_BASE = 192.0;
const float CLOUD_DEPTH = 22.0;

float cloudField(vec2 p, float coverage)
{
    vec2 drift = vec2(uTime * 0.9, uTime * 0.35);
    vec2 q = (p + drift) / 1024.0;
    float n = texture(uNoise, q).r * 0.52;
    n += texture(uNoise, q * 2.7 + 0.31).g * 0.28;
    n += texture(uNoise, q * 6.1 + 0.77).b * 0.14;
    n += texture(uNoise, q * 13.0 + 0.13).a * 0.06;
    return clamp((n - (1.0 - coverage)) / max(coverage, 0.05) * 1.6, 0.0, 1.0);
}

vec4 clouds(vec3 dir, vec3 sun)
{
    if (dir.y < 0.015) return vec4(0.0);

    float coverage = mix(0.40, 0.80, uRain);
    float jitter = ditherNoise(gl_FragCoord.xy);

    float density = 0.0;
    float lit = 0.0;
    const int STEPS = 4;
    for (int i = 0; i < STEPS; ++i)
    {
        float layer = (float(i) + jitter) / float(STEPS);
        float height = CLOUD_BASE + layer * CLOUD_DEPTH;
        float t = (height - uCameraPos.y) / dir.y;
        if (t <= 0.0) continue;
        vec2 p = uCameraPos.xz + dir.xz * t;
        float shape = cloudField(p, coverage) * (1.0 - abs(layer - 0.45) * 1.1);
        if (shape <= 0.0) continue;
        vec2 towardLight = p + sun.xz * 18.0 / max(abs(sun.y), 0.25);
        float occlusion = cloudField(towardLight, coverage);
        lit += shape * (1.0 - occlusion * 0.75) * (0.4 + 0.6 * layer);
        density += shape;
    }

    density /= float(STEPS);
    lit /= float(STEPS);
    if (density <= 0.0) return vec4(0.0);

    float alpha = 1.0 - exp(-density * 3.2);
    float fade = smoothstep(0.015, 0.20, dir.y);
    float silver = pow(max(dot(dir, sun), 0.0), 6.0);

    vec3 shadowSide = uSkyAmbient * 0.75 + vec3(0.02);
    vec3 sunSide = uLightColour * (0.55 + 1.6 * silver);
    vec3 colour = shadowSide + sunSide * lit / max(density, 0.001);
    colour = mix(colour, vec3(luma(colour)), uRain * 0.6);
    return vec4(colour, alpha * fade);
}

void main()
{
    vec4 farPoint = uInvViewProj * vec4(vUv * 2.0 - 1.0, 1.0, 1.0);
    vec3 dir = normalize(farPoint.xyz / farPoint.w - uCameraPos);
    vec3 sun = normalize(uSunDirection);

    vec3 colour = skyRadiance(dir, sun, uRain);

    float day = smoothstep(-0.10, 0.10, sun.y);
    float clear = 1.0 - uRain;

    if (dir.y > 0.0 && day < 1.0)
    {
        vec3 cell = floor(dir * 260.0);
        float star = hash12(cell.xy + cell.z * 17.31);
        float twinkle = 0.75 + 0.25 * sin(uTime * 3.0 + star * 60.0);
        float bright = smoothstep(0.9965, 1.0, star) * twinkle;
        colour += vec3(0.85, 0.90, 1.0) * bright * 1.6 * (1.0 - day) * clear * smoothstep(0.0, 0.25, dir.y);
    }

    float sunDot = dot(dir, sun);
    float disc = smoothstep(0.99935, 0.99965, sunDot);
    disc *= mix(0.72, 1.0, smoothstep(0.99935, 0.99995, sunDot));
    vec3 sunColour = mix(vec3(1.0, 0.45, 0.15), vec3(1.0, 0.93, 0.82), smoothstep(0.0, 0.35, sun.y));
    colour += sunColour * disc * 28.0 * clear * smoothstep(-0.05, 0.02, dir.y);

    float moonDot = dot(dir, -sun);
    float moon = smoothstep(0.99950, 0.99975, moonDot);
    float craters = 0.82 + 0.18 * texture(uNoise, dir.xz * 7.0).r;
    colour += vec3(0.75, 0.80, 0.92) * moon * craters * 2.4 * clear * smoothstep(-0.05, 0.02, dir.y);

    vec4 cloud = clouds(dir, sun.y > -0.05 ? sun : -sun);
    colour = mix(colour, cloud.rgb, cloud.a);

    FragColor = vec4(colour, 1.0);
}
