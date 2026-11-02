const float PI = 3.14159265;

const int MAT_SOLID = 0;
const int MAT_PLANT = 1;
const int MAT_LEAVES = 2;
const int MAT_WATER = 3;
const int MAT_LAVA = 4;
const int MAT_GLASS = 5;
const int MAT_ICE = 6;
const int MAT_SNOW = 7;
const int MAT_HANGING = 8;
const int MAT_METAL = 9;
const int MAT_ORE = 10;
const int MAT_GLOWING = 11;
const int MAT_POLISHED = 12;
const int MAT_FLAME = 13;
const int MAT_PORTAL = 14;
const int MAT_FLOATING = 15;
const int MAT_UNDERWATER_PLANT = 16;
const int MAT_TALL_PLANT = 17;

int tagFace(float tag) { return int(tag + 0.5) & 7; }
int tagBlock(float tag) { return (int(tag + 0.5) >> 3) & 1023; }
bool tagTop(float tag) { return int(tag + 0.5) >= 8192; }

float luma(vec3 c) { return dot(c, vec3(0.2126, 0.7152, 0.0722)); }

float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float ditherNoise(vec2 pixel)
{
    return fract(52.9829189 * fract(dot(pixel, vec2(0.06711056, 0.00583715))));
}

vec3 breeze(vec3 p, float t, float rain)
{
    float gust = 0.55 + 0.45 * sin(t * 0.31 + p.x * 0.021 + p.z * 0.017);
    gust *= 1.0 + rain * 0.9;
    float a = sin(t * 1.83 + p.x * 0.71 + p.z * 0.43 + p.y * 0.27);
    float b = sin(t * 1.21 - p.z * 0.66 + p.x * 0.29 + 1.7);
    float c = sin(t * 3.70 + p.x * 1.9 + p.z * 2.3) * 0.25;
    return vec3(a + c, 0.0, b - c) * gust;
}

float waterSwell(vec2 p, float t)
{
    float w = sin(p.x * 0.62 + t * 1.35) * 0.55 + sin(p.y * 0.51 - t * 1.05) * 0.45;
    w += sin((p.x + p.y) * 1.13 + t * 2.1) * 0.25;
    return w;
}

vec3 waveOffset(vec3 world, int material, bool top, float t, float rain)
{
    if (material == MAT_PLANT || material == MAT_TALL_PLANT)
        return top ? breeze(world, t, rain) * 0.085 : vec3(0.0);
    if (material == MAT_LEAVES)
    {
        vec3 b = breeze(world, t * 1.15, rain) * 0.04;
        b.y = sin(t * 2.3 + world.x * 1.7 + world.z * 1.3) * 0.015;
        return b;
    }
    if (material == MAT_HANGING) return breeze(world, t * 0.9, rain) * 0.045;
    if (material == MAT_UNDERWATER_PLANT)
        return top ? vec3(sin(t * 1.1 + world.x * 0.8), 0.0, cos(t * 0.9 + world.z * 0.7)) * 0.10
                   : vec3(0.0);
    if (material == MAT_WATER && top) return vec3(0.0, waterSwell(world.xz, t) * 0.028 - 0.03, 0.0);
    if (material == MAT_FLOATING) return vec3(0.0, waterSwell(world.xz, t) * 0.028 - 0.02, 0.0);
    if (material == MAT_LAVA && top) return vec3(0.0, sin(t * 0.7 + world.x * 0.9 + world.z * 0.6) * 0.02 - 0.02, 0.0);
    return vec3(0.0);
}

const float SHADOW_BEND = 0.85;

vec2 bendShadow(vec2 p)
{
    float d = length(p);
    return p / (d * SHADOW_BEND + (1.0 - SHADOW_BEND));
}

float bendScale(vec2 p)
{
    float d = length(p) * SHADOW_BEND + (1.0 - SHADOW_BEND);
    return d * d / (1.0 - SHADOW_BEND);
}

float duskAmount(float sunHeight)
{
    return smoothstep(-0.22, 0.0, sunHeight) * (1.0 - smoothstep(0.02, 0.38, sunHeight));
}

vec3 skyRadiance(vec3 dir, vec3 sun, float rain)
{
    float s = sun.y;
    float day = smoothstep(-0.12, 0.30, s);
    float dusk = duskAmount(s);
    float up = clamp(dir.y, 0.0, 1.0);
    float horizon = pow(1.0 - up, 3.5);

    vec3 zenithDay = vec3(0.16, 0.34, 0.86);
    vec3 horizonDay = vec3(0.60, 0.77, 1.00);
    vec3 zenithNight = vec3(0.004, 0.007, 0.020);
    vec3 horizonNight = vec3(0.014, 0.022, 0.050);

    vec3 zenith = mix(zenithNight, zenithDay, day);
    vec3 rim = mix(horizonNight, horizonDay, day);
    vec3 colour = mix(zenith, rim, horizon);

    float toward = dot(dir, sun) * 0.5 + 0.5;
    vec3 glow = mix(vec3(0.85, 0.30, 0.38), vec3(1.15, 0.50, 0.16), toward);
    colour = mix(colour, glow * (0.35 + 0.9 * pow(toward, 3.0)), dusk * horizon * 0.85);
    colour += vec3(0.30, 0.18, 0.42) * dusk * (1.0 - horizon) * 0.25;

    float halo = pow(max(dot(dir, sun), 0.0), 10.0);
    colour += vec3(1.0, 0.80, 0.58) * halo * (0.18 * day + 0.6 * dusk);

    float moonHalo = pow(max(dot(dir, -sun), 0.0), 18.0);
    colour += vec3(0.20, 0.26, 0.40) * moonHalo * (1.0 - day) * 0.25;

    if (dir.y < 0.0) colour *= mix(1.0, 0.35, clamp(-dir.y * 2.5, 0.0, 1.0));

    float grey = luma(colour);
    colour = mix(colour, vec3(grey) * vec3(0.92, 0.96, 1.0), rain * 0.85);
    colour *= 1.0 - rain * 0.45;
    return colour * 1.15;
}

vec3 waterFogColour(vec3 skyAmbient)
{
    return vec3(0.025, 0.11, 0.16) * (0.25 + luma(skyAmbient) * 0.9);
}
