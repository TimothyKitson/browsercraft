#version 330 core

in vec2 vUV;
in float vShade;
in float vSkyLight;
in float vBlockLight;
in float vViewDistance;
in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vTangent;
in vec3 vBitangent;
flat in int vIsCross;

uniform sampler2D uAtlas;
uniform sampler2D uNormalAtlas;   // LabPBR _n: normal.xy, ambient occlusion, height
uniform sampler2D uSpecularAtlas; // LabPBR _s: smoothness, F0, porosity, emission

uniform float uDaylight;     // 0 at midnight, 1 at noon
uniform vec3 uFogColor;
uniform float uFogStart;
uniform float uFogEnd;
uniform int uUnderwater;
uniform vec3 uSunDirection;
uniform vec3 uCameraPos;
uniform int uPbrEnabled;

out vec4 FragColor;

void main()
{
    vec4 texel = texture(uAtlas, vUV);
    if (texel.a < 0.1) discard; // leaf gaps, plant cut-outs

    // Baked voxel lighting: sunlight dims at night, torch light never does.
    float sky = vSkyLight * uDaylight;
    float level = max(sky, vBlockLight);
    float brightness = mix(0.07, 1.0, level);

    vec3 albedo = texel.rgb;
    vec3 color = albedo * brightness * vShade;

    if (uPbrEnabled == 1 && vIsCross == 0)
    {
        vec4 normalSample = texture(uNormalAtlas, vUV);
        vec4 specularSample = texture(uSpecularAtlas, vUV);

        // LabPBR stores only X and Y; Z is reconstructed as a unit vector.
        vec2 tangentXY = normalSample.rg * 2.0 - 1.0;
        float tangentZ = sqrt(clamp(1.0 - dot(tangentXY, tangentXY), 0.0, 1.0));
        vec3 normal = normalize(vTangent * tangentXY.x +
                                vBitangent * tangentXY.y +
                                vNormal * tangentZ);

        float materialAO = normalSample.b;
        float smoothness = specularSample.r;
        float reflectance = specularSample.g;
        // Alpha 255 means "not emissive"; 0-254 maps to 0..1.
        float emission = specularSample.a > 0.996 ? 0.0 : specularSample.a * (255.0 / 254.0);

        // Only surfaces the sky actually reaches get direct sun.
        float sunReach = vSkyLight * uDaylight;
        float incidence = max(dot(normal, uSunDirection), 0.0);
        float direct = incidence * sunReach;

        // Relight the surface: the normal map adds shape the baked
        // per-face shading can't express.
        float relief = mix(0.80, 1.22, direct);
        color = albedo * brightness * vShade * relief * mix(1.0, materialAO, 0.75);

        // Blinn-Phong highlight, sharpened by the pack's smoothness map.
        vec3 viewDir = normalize(uCameraPos - vWorldPos);
        vec3 halfway = normalize(viewDir + uSunDirection);
        float specularPower = exp2(1.0 + smoothness * 9.0);
        float highlight = pow(max(dot(normal, halfway), 0.0), specularPower);
        color += vec3(1.0, 0.97, 0.90) * highlight * smoothness * (0.04 + reflectance) * direct * 2.0;

        color += albedo * emission * 1.6;
    }

    float fogAmount = clamp((vViewDistance - uFogStart) / max(uFogEnd - uFogStart, 0.001), 0.0, 1.0);
    vec3 fogColor = uFogColor;

    if (uUnderwater == 1)
    {
        color = mix(color, vec3(0.10, 0.30, 0.55), 0.45);
        fogColor = vec3(0.07, 0.21, 0.42);
        fogAmount = clamp(vViewDistance / 24.0, 0.0, 1.0);
    }

    color = mix(color, fogColor, fogAmount);
    FragColor = vec4(color, texel.a);
}
