#version 330 core

in vec2 vUV;

uniform sampler2D uTexture;
uniform vec4 uColor;

out vec4 FragColor;

void main()
{
    vec4 color = texture(uTexture, vUV);
    if (color.a == 0.0) discard;
    FragColor = color * uColor;
}
