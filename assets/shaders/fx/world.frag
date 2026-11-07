#version 330 core
#include "common.glsl"

in vec2 vUV;
in vec3 vTint;
in float vTintSelective;
in float vShade;
in float vSkyLight;
in float vBlockLight;
in vec3 vWorldPos;
in vec3 vViewPos;
flat in int vFace;
flat in int vBlock;
flat in int vMaterial;
flat in vec4 vInfo;

uniform sampler2D uAtlas;
uniform sampler2D uNormalAtlas;
uniform sampler2D uSpecularAtlas;
uniform sampler2DShadow uShadowMap;
uniform sampler2D uNoise;
uniform sampler2D uSceneCopy;
uniform sampler2D uDepthCopy;

uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uShadowViewProj;
uniform vec3 uCameraPos;
uniform vec3 uSunDirection;
uniform vec3 uLightDir;
uniform vec3 uLightColour;
uniform vec3 uSkyAmbient;
uniform float uShadowStrength;
uniform float uDaylight;
uniform float uAmbient;
uniform float uRain;
uniform float uTime;
uniform int uDimension;
uniform int uPass;
uniform int uUnderwater;
uniform int uPbrEnabled;
uniform vec2 uScreenSize;
uniform float uNear;
uniform float uFar;

out vec4 FragColor;

const vec3 FACE_NORMAL[6] = vec3[6](
    vec3(1.0, 0.0, 0.0), vec3(-1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0), vec3(0.0, -1.0, 0.0),
    vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, -1.0));
const vec3 FACE_TANGENT[6] = vec3[6](
    vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 1.0),
    vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0),
    vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0));
const vec3 FACE_BITANGENT[6] = vec3[6](
    vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, -1.0), vec3(0.0, 0.0, -1.0),
    vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0));
const float FACE_SHADE[6] = float[6](0.72, 0.72, 1.0, 0.5, 0.86, 0.86);

const vec2 DISC[12] = vec2[12](
    vec2(0.9439, 0.0812), vec2(-0.3471, 0.7955), vec2(-0.7653, -0.4316), vec2(0.2817, -0.8724),
    vec2(0.4720, 0.4113), vec2(-0.5103, 0.1802), vec2(0.0621, -0.4392), vec2(0.2133, 0.1520),
    vec2(-0.1504, 0.4720), vec2(0.6632, -0.3810), vec2(-0.3920, -0.7712), vec2(-0.9010, 0.2210));

float linearDepth(float d)
{
    float z = d * 2.0 - 1.0;
    return 2.0 * uNear * uFar / (uFar + uNear - z * (uFar - uNear));
}

float shadowAt(vec3 world, vec3 normal)
{
    vec4 probe = uShadowViewProj * vec4(world, 1.0);
    float texelWorld = 2.0 * 96.0 / 2048.0 * bendScale(probe.xy);
    vec3 shifted = world + normal * (0.03 + texelWorld * 1.1);

    vec4 clip = uShadowViewProj * vec4(shifted, 1.0);
    float edge = max(abs(clip.x), abs(clip.y));
    if (edge >= 1.0 || clip.z > 1.0 || clip.z < -1.0) return 1.0;

    vec2 bent = bendShadow(clip.xy);
    vec3 coord = vec3(bent * 0.5 + 0.5, clip.z * 0.5 + 0.5 - 0.0002);

    float radius = 1.4 / 2048.0;

    float lit = 0.0;
    for (int i = 0; i < 12; ++i)
        lit += texture(uShadowMap, vec3(coord.xy + DISC[i] * radius, coord.z));
    lit /= 12.0;

    return mix(lit, 1.0, smoothstep(0.80, 0.98, edge));
}

vec3 waterNormal(vec2 p)
{
    float t = uTime;
    vec2 a = p * 0.045 + vec2(t * 0.011, t * 0.006);
    vec2 b = p * 0.110 + vec2(-t * 0.017, t * 0.013);
    vec2 c = p * 0.260 + vec2(t * 0.021, -t * 0.024);
    float e = 0.04;
    float hx = texture(uNoise, a + vec2(e, 0.0)).r - texture(uNoise, a - vec2(e, 0.0)).r;
    float hz = texture(uNoise, a + vec2(0.0, e)).r - texture(uNoise, a - vec2(0.0, e)).r;
    hx += (texture(uNoise, b + vec2(e, 0.0)).g - texture(uNoise, b - vec2(e, 0.0)).g) * 0.6;
    hz += (texture(uNoise, b + vec2(0.0, e)).g - texture(uNoise, b - vec2(0.0, e)).g) * 0.6;
    hx += (texture(uNoise, c + vec2(e, 0.0)).b - texture(uNoise, c - vec2(e, 0.0)).b) * 0.35;
    hz += (texture(uNoise, c + vec2(0.0, e)).b - texture(uNoise, c - vec2(0.0, e)).b) * 0.35;
    float bump = 0.55 * (1.0 + uRain * 0.6);
    return normalize(vec3(-hx * bump, 1.0, -hz * bump));
}

bool traceScreen(vec3 origin, vec3 dir, out vec2 hitUv)
{
    vec3 ray = origin;
    float stride = 0.35 + length(origin) * 0.03;
    vec3 stepRay = dir * stride;
    float jitter = ditherNoise(gl_FragCoord.xy);
    ray += stepRay * jitter;

    for (int i = 0; i < 24; ++i)
    {
        ray += stepRay;
        vec4 clip = uProjection * vec4(ray, 1.0);
        if (clip.w <= 0.0) return false;
        vec2 uv = clip.xy / clip.w * 0.5 + 0.5;
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) return false;

        float scene = linearDepth(texture(uDepthCopy, uv).r);
        float depth = -ray.z;
        float gap = depth - scene;
        if (gap > 0.0 && gap < length(stepRay) * 2.5 + 0.4)
        {
            vec3 lo = ray - stepRay;
            vec3 hi = ray;
            for (int j = 0; j < 5; ++j)
            {
                vec3 mid = (lo + hi) * 0.5;
                vec4 m = uProjection * vec4(mid, 1.0);
                vec2 muv = m.xy / m.w * 0.5 + 0.5;
                if (-mid.z > linearDepth(texture(uDepthCopy, muv).r)) hi = mid;
                else lo = mid;
            }
            vec4 h = uProjection * vec4(hi, 1.0);
            hitUv = h.xy / h.w * 0.5 + 0.5;
            return texture(uDepthCopy, hitUv).r < 0.99999;
        }
        stepRay *= 1.28;
    }
    return false;
}

vec3 skyLit(vec3 dir)
{
    return skyRadiance(dir, normalize(uSunDirection), uRain);
}

vec3 shadeWater(vec3 albedoTex, vec3 viewDir, float sky)
{
    vec3 n = waterNormal(vWorldPos.xz);
    bool topFace = vFace == 2;
    if (!topFace) n = FACE_NORMAL[clamp(vFace, 0, 5)];

    bool below = uUnderwater == 1;
    if (below) n = -n;

    float cosV = max(dot(n, viewDir), 0.0);
    float fresnel = 0.02 + 0.98 * pow(1.0 - cosV, 5.0);

    vec2 screen = gl_FragCoord.xy / uScreenSize;
    float surface = linearDepth(gl_FragCoord.z);
    float behindDepth = linearDepth(texture(uDepthCopy, screen).r);
    float thickness = max(behindDepth - surface, 0.0);

    vec3 viewNormal = mat3(uView) * n;
    vec2 bend = viewNormal.xy * 0.045 * clamp(thickness * 0.35, 0.0, 1.0);
    vec2 refractUv = clamp(screen + bend, vec2(0.001), vec2(0.999));
    float refractDepth = linearDepth(texture(uDepthCopy, refractUv).r);
    if (refractDepth < surface) refractUv = screen;
    else thickness = max(refractDepth - surface, 0.0);

    vec3 behind = texture(uSceneCopy, refractUv).rgb;
    vec3 fog = waterFogColour(uSkyAmbient) * (0.15 + 0.85 * sky);
    vec3 absorb = exp(-thickness * vec3(0.38, 0.085, 0.060));
    vec3 body = behind * absorb + fog * (1.0 - absorb.g);
    body *= mix(vec3(1.0), albedoTex * 1.6, 0.18);

    if (below)
    {
        float through = smoothstep(0.0, 0.25, cosV);
        return mix(fog * 0.6, behind * vec3(0.55, 0.85, 0.95), through);
    }

    vec3 reflectDir = reflect(-viewDir, n);
    vec3 reflection = skyLit(reflectDir) * mix(0.08, 1.0, sky * sky);
    vec3 viewReflect = normalize(mat3(uView) * reflectDir);
    vec2 hitUv;
    if (topFace && traceScreen(vViewPos, viewReflect, hitUv))
    {
        vec2 fadeEdge = smoothstep(vec2(0.0), vec2(0.08), hitUv) * (1.0 - smoothstep(vec2(0.92), vec2(1.0), hitUv));
        reflection = mix(reflection, texture(uSceneCopy, hitUv).rgb, fadeEdge.x * fadeEdge.y);
    }

    vec3 colour = mix(body, reflection, fresnel);

    if (topFace)
    {
        float edge = 1.0 - smoothstep(0.05, 0.55, thickness);
        float bubbles = texture(uNoise, vWorldPos.xz * 0.9 + vec2(uTime * 0.05, -uTime * 0.04)).a;
        float foam = edge * smoothstep(0.45, 0.75, bubbles + edge * 0.35);
        vec3 foamLight = uSkyAmbient * sky * sky + uLightColour * 0.6 * smoothstep(0.5, 0.9, sky);
        colour = mix(colour, vec3(0.85, 0.92, 0.95) * foamLight, foam * 0.7);
    }

    vec3 h = normalize(uLightDir + viewDir);
    float spec = pow(max(dot(n, h), 0.0), 220.0) * 18.0;
    float sun = uShadowStrength > 0.0 ? shadowAt(vWorldPos, vec3(0.0, 1.0, 0.0)) : 1.0;
    colour += uLightColour * spec * fresnel * sun * smoothstep(0.5, 0.9, sky) * (1.0 - uRain);
    return colour;
}

void main()
{
    vec2 uv = vUV;
    vec4 texel = texture(uAtlas, uv);
    if (texel.a < 0.1) discard;
    vec3 biomeTint = vTintSelective > 0.5 ? mix(vec3(1.0), vTint, step(texel.r + 0.02, texel.g)) : vTint;
    texel.rgb *= biomeTint;

    int material = vMaterial;
    bool chunk = vBlock > 0;
    bool plantQuad = vFace == 6;
    vec3 viewDir = normalize(uCameraPos - vWorldPos);

    vec3 geoNormal;
    vec3 tangent = vec3(1.0, 0.0, 0.0);
    vec3 bitangent = vec3(0.0, 0.0, -1.0);
    if (chunk && vFace < 6)
    {
        geoNormal = FACE_NORMAL[vFace];
        tangent = FACE_TANGENT[vFace];
        bitangent = FACE_BITANGENT[vFace];
    }
    else if (chunk && plantQuad)
    {
        geoNormal = vec3(0.0, 1.0, 0.0);
    }
    else
    {
        geoNormal = normalize(cross(dFdx(vWorldPos), dFdy(vWorldPos)));
        if (dot(geoNormal, viewDir) < 0.0) geoNormal = -geoNormal;
        if (any(isnan(geoNormal))) geoNormal = vec3(0.0, 1.0, 0.0);
    }

    vec3 albedo = pow(texel.rgb, vec3(2.2));
    float sky = clamp(vSkyLight, 0.0, 1.0);
    float torch = clamp(vBlockLight, 0.0, 1.0);

    float ao = 1.0;
    if (chunk && vFace < 6) ao = clamp(vShade / FACE_SHADE[vFace], 0.0, 1.0);
    else if (!chunk) ao = mix(1.0, clamp(vShade, 0.0, 1.0), 0.35);

    if (material == MAT_WATER && uPass == 1)
    {
        vec3 waterTex = pow(texel.rgb, vec3(2.2));
        FragColor = vec4(shadeWater(waterTex, viewDir, sky), 1.0);
        return;
    }

    vec3 normal = geoNormal;
    float smoothness = vInfo.b;
    float metal = vInfo.a;
    float emission = vInfo.g;

    if (uPbrEnabled == 1 && chunk && !plantQuad)
    {
        vec4 normalSample = texture(uNormalAtlas, uv);
        vec4 specularSample = texture(uSpecularAtlas, uv);
        vec2 xy = normalSample.rg * 2.0 - 1.0;
        float z = sqrt(clamp(1.0 - dot(xy, xy), 0.0, 1.0));
        normal = normalize(tangent * xy.x + bitangent * xy.y + geoNormal * z);
        ao *= mix(1.0, normalSample.b, 0.6);
        smoothness = max(smoothness, specularSample.r);
        metal = max(metal, specularSample.g > 0.9 ? 1.0 : specularSample.g);
        float pbrEmission = specularSample.a > 0.996 ? 0.0 : specularSample.a;
        emission = max(emission, pbrEmission);
    }

    float wet = 0.0;
    if (uRain > 0.0 && uDimension == 0 && chunk)
    {
        float exposed = smoothstep(0.86, 0.97, sky) * max(geoNormal.y, 0.0);
        float patches = smoothstep(0.42, 0.62, texture(uNoise, vWorldPos.xz * 0.018).g);
        wet = uRain * exposed * mix(0.45, 1.0, patches);
        if (material == MAT_PLANT || material == MAT_LEAVES || material == MAT_TALL_PLANT) wet *= 0.3;
        albedo *= 1.0 - wet * 0.32;
        smoothness = max(smoothness, wet * 0.82);
    }

    float NdotL = dot(normal, uLightDir);
    bool foliage = material == MAT_PLANT || material == MAT_LEAVES || material == MAT_HANGING ||
                   material == MAT_TALL_PLANT || material == MAT_UNDERWATER_PLANT ||
                   material == MAT_FLOATING;

    float diffuse = foliage ? mix(0.55, 1.0, max(NdotL, 0.0)) : max((NdotL + 0.28) / 1.28, 0.0);
    if (plantQuad) diffuse = 0.55 + 0.45 * max(uLightDir.y, 0.0);

    float shadow = 1.0;
    if (uShadowStrength > 0.0 && uPass != 2 && diffuse > 0.0 && sky > 0.05)
        shadow = shadowAt(vWorldPos, foliage ? vec3(0.0, 1.0, 0.0) : geoNormal);

    float sunReach = smoothstep(0.30, 0.80, sky);
    if (uPass == 2) sunReach = smoothstep(0.30, 0.90, sky);
    vec3 direct = uLightColour * diffuse * mix(1.0, shadow, uShadowStrength) * sunReach;
    if (uUnderwater == 1 && uPass != 2)
    {
        vec2 q = vWorldPos.xz * 0.35 + vWorldPos.y * 0.12;
        float caustic = texture(uNoise, q * 0.21 + vec2(uTime * 0.020, uTime * 0.013)).g +
                        texture(uNoise, q * 0.17 - vec2(uTime * 0.017, -uTime * 0.021)).b;
        caustic = pow(1.0 - abs(caustic - 1.0), 6.0);
        direct *= vec3(0.16, 0.42, 0.48) * (0.55 + 1.6 * caustic);
    }

    if (foliage)
    {
        float behind = pow(max(dot(-viewDir, uLightDir), 0.0), 6.0);
        direct += uLightColour * albedo * behind * 1.4 * shadow * sunReach;
    }

    float skyTerm = sky * sky;
    float faceLight = mix(0.58, 1.0, geoNormal.y * 0.5 + 0.5) * (1.0 - 0.10 * abs(geoNormal.x));
    vec3 ambient = uSkyAmbient * skyTerm * faceLight * (0.85 + 0.15 * normal.y);

    vec3 torchColour = vec3(1.0, 0.58, 0.26);
    vec3 torchLight = torchColour * (pow(torch, 3.0) * 1.25 + pow(torch, 1.5) * 0.10);

    vec3 floorLight = vec3(0.006, 0.007, 0.011) * (1.0 - sky);
    if (uDimension == 1) floorLight = uSkyAmbient * 0.22 + vec3(0.02, 0.008, 0.004);
    if (uDimension == 2) floorLight = uSkyAmbient * 0.20;

    vec3 light = direct + (ambient + (torchLight + floorLight) * (0.55 + 0.45 * faceLight)) * ao;
    vec3 colour = albedo * light;

    vec3 f0 = mix(vec3(0.04), albedo, metal);
    if (smoothness > 0.05)
    {
        vec3 h = normalize(uLightDir + viewDir);
        float power = exp2(2.0 + smoothness * 10.0);
        float normaliser = (power + 8.0) / (8.0 * PI);
        vec3 fresnel = f0 + (1.0 - f0) * pow(1.0 - max(dot(h, viewDir), 0.0), 5.0);
        float highlight = normaliser * pow(max(dot(normal, h), 0.0), power);
        colour += uLightColour * fresnel * highlight * max(NdotL, 0.0) * shadow * sunReach *
                  smoothness * 0.6;

        vec3 viewFresnel = f0 + (1.0 - f0) * pow(1.0 - max(dot(normal, viewDir), 0.0), 5.0);
        vec3 reflectDir = reflect(-viewDir, normal);
        vec3 env = skyLit(reflectDir) * skyTerm;
        colour = mix(colour, env, viewFresnel * smoothness * smoothness * 0.55);
        colour *= 1.0 - metal * 0.35 * smoothness;
    }

    float texLuma = luma(texel.rgb);
    if (material == MAT_LAVA)
        colour = albedo * 4.2 + albedo * light * 0.2;
    else if (material == MAT_FLAME)
        colour += albedo * 3.2 * smoothstep(0.25, 0.75, texLuma);
    else if (material == MAT_PORTAL)
        colour += albedo * 2.8;
    else if (material == MAT_GLOWING)
    {
        float warm = mix(0.5, 1.0, smoothstep(0.0, 0.2, texel.r - texel.b));
        colour += albedo * emission * 3.4 * smoothstep(0.30, 0.85, texLuma) * warm;
    }
    else if (material == MAT_ORE)
    {
        float saturation = max(max(texel.r, texel.g), texel.b) - min(min(texel.r, texel.g), texel.b);
        float mask = smoothstep(0.18, 0.42, saturation) * smoothstep(0.25, 0.65, texLuma);
        colour += albedo * mask * 0.55 * (1.0 - sky * uDaylight * 0.6);
    }
    else if (emission > 0.0)
        colour += albedo * emission * 2.0;

    if (material == MAT_HANGING && emission > 0.0)
        colour += albedo * 2.0 * smoothstep(0.55, 0.85, texLuma);

    float alpha = texel.a;
    if (uPass == 1 && (material == MAT_GLASS || material == MAT_ICE))
    {
        float cosV = max(dot(normal, viewDir), 0.0);
        float fresnel = 0.04 + 0.96 * pow(1.0 - cosV, 5.0);
        vec3 env = skyLit(reflect(-viewDir, normal)) * mix(0.1, 1.0, skyTerm);
        colour = mix(colour, env, fresnel * 0.7);
        alpha = max(alpha, fresnel * 0.6);
    }

    FragColor = vec4(colour, alpha);
}
