#version 330 core
#include "common.glsl"

in vec2 vUv;
uniform sampler2D uImage;
uniform vec2 uTexel;

out vec4 FragColor;

void main()
{
    vec3 centre = texture(uImage, vUv).rgb;
    float lc = luma(centre);
    float nw = luma(texture(uImage, vUv + vec2(-1.0, 1.0) * uTexel).rgb);
    float ne = luma(texture(uImage, vUv + vec2(1.0, 1.0) * uTexel).rgb);
    float sw = luma(texture(uImage, vUv + vec2(-1.0, -1.0) * uTexel).rgb);
    float se = luma(texture(uImage, vUv + vec2(1.0, -1.0) * uTexel).rgb);

    float lo = min(lc, min(min(nw, ne), min(sw, se)));
    float hi = max(lc, max(max(nw, ne), max(sw, se)));
    if (hi - lo < max(0.045, hi * 0.15))
    {
        FragColor = vec4(centre, 1.0);
        return;
    }

    vec2 dir = vec2(-((nw + ne) - (sw + se)), (nw + sw) - (ne + se));
    float reduce = max((nw + ne + sw + se) * 0.03125, 0.0078125);
    float scale = 1.0 / (min(abs(dir.x), abs(dir.y)) + reduce);
    dir = clamp(dir * scale, vec2(-6.0), vec2(6.0)) * uTexel;

    vec3 inner = 0.5 * (texture(uImage, vUv + dir * (1.0 / 3.0 - 0.5)).rgb +
                       texture(uImage, vUv + dir * (2.0 / 3.0 - 0.5)).rgb);
    vec3 wide = inner * 0.5 + 0.25 * (texture(uImage, vUv - dir * 0.5).rgb +
                                     texture(uImage, vUv + dir * 0.5).rgb);
    float lw = luma(wide);
    FragColor = vec4((lw < lo || lw > hi) ? inner : wide, 1.0);
}
