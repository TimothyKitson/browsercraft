#version 330 core

in vec2 vUv;
uniform sampler2D uSource;
uniform vec2 uTexel;

out vec4 FragColor;

void main()
{
    vec3 c = texture(uSource, vUv).rgb * 4.0;
    c += texture(uSource, vUv + vec2(uTexel.x, 0.0)).rgb * 2.0;
    c += texture(uSource, vUv - vec2(uTexel.x, 0.0)).rgb * 2.0;
    c += texture(uSource, vUv + vec2(0.0, uTexel.y)).rgb * 2.0;
    c += texture(uSource, vUv - vec2(0.0, uTexel.y)).rgb * 2.0;
    c += texture(uSource, vUv + uTexel).rgb;
    c += texture(uSource, vUv - uTexel).rgb;
    c += texture(uSource, vUv + vec2(uTexel.x, -uTexel.y)).rgb;
    c += texture(uSource, vUv + vec2(-uTexel.x, uTexel.y)).rgb;
    FragColor = vec4(c / 16.0 * 0.85, 1.0);
}
