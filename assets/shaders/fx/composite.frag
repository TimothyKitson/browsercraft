#version 330 core
#include "common.glsl"

in vec2 vUv;

uniform sampler2D uScene;
uniform sampler2D uDepth;
uniform sampler2D uHandDepth;
uniform sampler2DShadow uShadowMap;
uniform sampler2D uNoise;

uniform mat4 uInvProjection;
uniform mat4 uInvView;
uniform mat4 uShadowViewProj;
uniform vec3 uCameraPos;
uniform vec3 uSunDirection;
uniform vec3 uLightDir;
uniform vec3 uLightColour;
uniform vec3 uSkyAmbient;
uniform vec3 uBackdrop;
uniform float uShadowStrength;
uniform float uRain;
uniform float uTime;
uniform float uRenderDistance;
uniform float uEyeSkyLight;
uniform int uDimension;
uniform int uUnderwater;
uniform int uInLava;
uniform vec2 uScreenSize;

out vec4 FragColor;

float lightAt(vec3 world)
{
    vec4 clip = uShadowViewProj * vec4(world, 1.0);
    if (max(abs(clip.x), abs(clip.y)) >= 1.0) return 1.0;
    vec2 bent = bendShadow(clip.xy);
    return texture(uShadowMap, vec3(bent * 0.5 + 0.5, clip.z * 0.5 + 0.5 - 0.0005));
}

float phase(float cosTheta, float g)
{
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * PI * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
}

void main()
{
    vec3 colour = texture(uScene, vUv).rgb;

    if (texture(uHandDepth, vUv).r < 1.0)
    {
        FragColor = vec4(colour, 1.0);
        return;
    }

    float depth = texture(uDepth, vUv).r;
    vec4 view = uInvProjection * vec4(vUv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec3 viewPos = view.xyz / view.w;
    vec3 dir = normalize(mat3(uInvView) * viewPos);
    bool sky = depth >= 0.99999;
    float dist = sky ? uRenderDistance * 1.2 : length(viewPos);
    vec3 sun = normalize(uSunDirection);

    if (uDimension == 0)
    {
        if (!sky)
        {
            vec3 fogColour = skyRadiance(dir, sun, uRain);
            float border = smoothstep(uRenderDistance * 0.60, uRenderDistance * 0.97, dist);
            float heightFade = exp(-max(uCameraPos.y + dir.y * dist * 0.5 - 62.0, 0.0) / 48.0);
            float density = (0.0011 + uRain * 0.0065 + duskAmount(sun.y) * 0.0012) * heightFade;
            float haze = 1.0 - exp(-dist * density);
            float caves = mix(0.08, 1.0, uEyeSkyLight);
            colour = mix(colour, fogColour * caves, haze);
            colour = mix(colour, fogColour, border);
        }

        if (uShadowStrength > 0.0 && uUnderwater == 0 && uEyeSkyLight > 0.01)
        {
            float reach = min(dist, 80.0);
            const int STEPS = 9;
            float jitter = hash12(gl_FragCoord.xy + fract(uTime) * 61.0);
            float lit = 0.0;
            for (int i = 0; i < STEPS; ++i)
            {
                float t = (float(i) + jitter) / float(STEPS);
                lit += lightAt(uCameraPos + dir * reach * t * t);
            }
            lit /= float(STEPS);

            float cosTheta = dot(dir, uLightDir);
            float scatter = min(phase(cosTheta, 0.65), 1.2) * 0.9 + 0.035;
            float morning = duskAmount(sun.y);
            float density = 0.0009 + morning * 0.0065 + uRain * 0.002;
            float amount = 1.0 - exp(-reach * density);
            colour += uLightColour * lit * scatter * amount * uShadowStrength * uEyeSkyLight;
        }
    }
    else
    {
        float density = uDimension == 1 ? 0.0085 : 0.006;
        float haze = 1.0 - exp(-dist * density);
        float border = smoothstep(uRenderDistance * 0.55, uRenderDistance * 0.95, dist);
        if (!sky) colour = mix(colour, uBackdrop * 0.6, max(haze * 0.7, border));
    }

    if (uUnderwater == 1)
    {
        vec3 fog = waterFogColour(uSkyAmbient) * (0.2 + 0.8 * uEyeSkyLight);
        float amount = 1.0 - exp(-dist * 0.075);
        colour = mix(colour * vec3(0.55, 0.85, 0.95), fog, amount);
    }

    if (uInLava == 1)
    {
        float amount = 1.0 - exp(-dist * 0.9);
        colour = mix(colour, vec3(1.6, 0.45, 0.06), amount);
    }

    FragColor = vec4(colour, 1.0);
}
