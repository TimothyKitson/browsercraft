#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in float aAlpha;

uniform mat4 uView;
uniform mat4 uProjection;

out vec2 vUV;
out float vAlpha;

void main()
{
    vUV = aUV;
    vAlpha = aAlpha;
    gl_Position = uProjection * uView * vec4(aPos, 1.0);
}
