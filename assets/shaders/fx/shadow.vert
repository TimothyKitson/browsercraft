#version 330 core
#include "common.glsl"

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in float aShade;
layout (location = 3) in float aSkyLight;
layout (location = 4) in float aBlockLight;
layout (location = 5) in float aFace;

uniform mat4 uShadowViewProj;
uniform vec3 uChunkOffset;
uniform sampler2D uMaterials;
uniform float uTime;
uniform float uRain;

out vec2 vUV;

void main()
{
    int block = tagBlock(aFace);
    int material = int(texelFetch(uMaterials, ivec2(block, 0), 0).r * 255.0 + 0.5);
    vec3 world = aPos + uChunkOffset;
    world += waveOffset(world, material, tagTop(aFace), uTime, uRain);

    vec4 clip = uShadowViewProj * vec4(world, 1.0);
    clip.xy = bendShadow(clip.xy);
    gl_Position = clip;
    vUV = aUV;
}
