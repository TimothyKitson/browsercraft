#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec4 aColor;
layout (location = 3) in vec3 aLight;

uniform mat4 uProjection;

out vec2 vUV;
out vec4 vColor;
out vec3 vLight;

void main()
{
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    vUV = aUV;
    vColor = aColor;
    vLight = aLight;
}
