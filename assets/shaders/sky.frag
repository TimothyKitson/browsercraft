#version 330 core

in vec2 vNdc;

uniform mat4 uInvViewProj;
uniform vec3 uSunDirection;
uniform vec3 uCameraPos;
uniform float uDayFactor; // 0 = midnight, 1 = noon
uniform float uTime;      // seconds, for cloud drift
uniform float uOvercast;  // 0 = clear, 1 = full rain

out vec4 FragColor;

float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float valueNoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash12(i);
    float b = hash12(i + vec2(1.0, 0.0));
    float c = hash12(i + vec2(0.0, 1.0));
    float d = hash12(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float cloudNoise(vec2 p)
{
    float total = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 4; ++i)
    {
        total += valueNoise(p) * amplitude;
        p *= 2.1;
        amplitude *= 0.5;
    }
    return total;
}

void main()
{
    // Turn this pixel back into a world-space view ray.
    vec4 farPoint = uInvViewProj * vec4(vNdc, 1.0, 1.0);
    vec3 dir = normalize(farPoint.xyz / farPoint.w);

    float height = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);

    vec3 dayZenith = vec3(0.24, 0.48, 0.92);
    vec3 dayHorizon = vec3(0.70, 0.84, 0.98);
    vec3 nightZenith = vec3(0.015, 0.025, 0.08);
    vec3 nightHorizon = vec3(0.06, 0.08, 0.17);

    vec3 zenith = mix(nightZenith, dayZenith, uDayFactor);
    vec3 horizon = mix(nightHorizon, dayHorizon, uDayFactor);

    // Rain takes the blue out of the sky. This is most of what makes
    // weather read as weather: the world dimming on its own just looks
    // like dusk coming early.
    vec3 wetZenith = mix(vec3(0.10, 0.11, 0.13), vec3(0.42, 0.44, 0.48), uDayFactor);
    vec3 wetHorizon = mix(vec3(0.13, 0.14, 0.16), vec3(0.62, 0.64, 0.67), uDayFactor);
    zenith = mix(zenith, wetZenith, uOvercast);
    horizon = mix(horizon, wetHorizon, uOvercast);

    // Warm the horizon while the sun sits near it.
    // No red sunsets through cloud, either.
    float sunset = clamp(1.0 - abs(uSunDirection.y) * 4.0, 0.0, 1.0) * uDayFactor
                 * (1.0 - uOvercast);
    horizon = mix(horizon, vec3(0.96, 0.56, 0.28), sunset * 0.75);

    vec3 color = mix(horizon, zenith, pow(height, 0.6));

    // Stars, fading in as the sun goes down.
    // Cloud hides the stars, the sun and the moon alike.
    float clearSky = 1.0 - uOvercast;
    float nightAmount = clamp(1.0 - uDayFactor * 1.6, 0.0, 1.0) * clearSky;
    if (nightAmount > 0.01 && dir.y > -0.05)
    {
        vec2 starGrid = floor(dir.xz / max(abs(dir.y) + 0.12, 0.12) * 110.0);
        float star = hash12(starGrid);
        float twinkle = 0.75 + 0.25 * sin(uTime * 2.0 + star * 40.0);
        if (star > 0.9965)
            color += vec3(1.0, 0.98, 0.92) * nightAmount * twinkle;
    }

    // Sun and moon discs.
    float sunDot = dot(dir, uSunDirection);
    color += vec3(1.0, 0.95, 0.82) * pow(max(sunDot, 0.0), 3000.0) * 12.0 * clearSky;
    color += vec3(1.0, 0.82, 0.55) * pow(max(sunDot, 0.0), 80.0) * 0.22 * uDayFactor * clearSky;

    float moonDot = dot(dir, -uSunDirection);
    color += vec3(0.88, 0.90, 1.0) * pow(max(moonDot, 0.0), 4000.0) * 9.0 * nightAmount;

    // A drifting cloud layer, found by intersecting the ray with a plane.
    if (dir.y > 0.015)
    {
        float cloudPlaneY = 164.0;
        float t = (cloudPlaneY - uCameraPos.y) / dir.y;
        if (t > 0.0 && t < 12000.0)
        {
            vec2 cloudPos = (uCameraPos.xz + dir.xz * t) * 0.0016 + vec2(uTime * 0.004, uTime * 0.0015);
            float density = cloudNoise(cloudPos);
            // Thicker and lower-contrast as the rain comes in, until
            // the layer is solid overhead.
            float coverage = smoothstep(mix(0.52, 0.10, uOvercast),
                                        mix(0.78, 0.30, uOvercast), density);
            // Fade clouds out towards the horizon so the plane edge is hidden.
            coverage *= smoothstep(0.015, 0.22, dir.y);
            vec3 cloudColor = mix(vec3(0.58, 0.62, 0.72), vec3(1.0, 0.99, 0.96), uDayFactor);
            cloudColor = mix(cloudColor, vec3(1.0, 0.76, 0.55), sunset * 0.6);
            cloudColor = mix(cloudColor,
                             mix(vec3(0.09, 0.10, 0.12), vec3(0.40, 0.42, 0.46), uDayFactor),
                             uOvercast);
            color = mix(color, cloudColor, coverage * mix(0.85, 1.0, uOvercast));
        }
    }

    FragColor = vec4(color, 1.0);
}
