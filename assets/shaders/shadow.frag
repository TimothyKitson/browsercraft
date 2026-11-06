#version 330 core

uniform sampler2D uTexture;

in vec2 vUV;
in float vAlpha;

out vec4 FragColor;

void main()
{
    float a = texture(uTexture, vUV).a * vAlpha;
    if (a < 0.004) discard;
    FragColor = vec4(0.0, 0.0, 0.0, a);
}
