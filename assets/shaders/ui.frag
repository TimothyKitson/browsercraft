#version 330 core

in vec2 vUV;
in vec4 vColor;

uniform sampler2D uTexture;

out vec4 FragColor;

void main()
{
    vec4 texel = texture(uTexture, vUV);
    FragColor = texel * vColor;
    if (FragColor.a < 0.004) discard;
}
