#version 330 core
#include "common.glsl"

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in float aShade;
layout (location = 3) in float aSkyLight;
layout (location = 4) in float aBlockLight;
layout (location = 5) in float aFace;
layout (location = 6) in float aTint;

uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uChunkOffset;
uniform sampler2D uMaterials;
uniform float uTime;
uniform float uRain;

out vec2 vUV;
out vec3 vTint;
out float vTintSelective;
out float vShade;
out float vSkyLight;
out float vBlockLight;
out vec3 vWorldPos;
out vec3 vViewPos;
flat out int vFace;
flat out int vBlock;
flat out int vMaterial;
flat out vec4 vInfo;

void main()
{
    uint packedTint = uint(max(aTint, 0.0) + 0.5);
    vTint = vec3(float((packedTint >> 14u) & 127u), float((packedTint >> 7u) & 127u), float(packedTint & 127u)) / 64.0;
    vTintSelective = float((packedTint >> 21u) & 1u);
    int face = tagFace(aFace);
    int block = tagBlock(aFace);
    vec4 info = texelFetch(uMaterials, ivec2(block, 0), 0);
    int material = int(info.r * 255.0 + 0.5);

    vec3 world = aPos + uChunkOffset;
    if (block > 0) world += waveOffset(world, material, tagTop(aFace), uTime, uRain);

    vec4 view = uView * vec4(world, 1.0);
    gl_Position = uProjection * view;

    vUV = aUV;
    vShade = aShade;
    vSkyLight = aSkyLight;
    vBlockLight = aBlockLight;
    vWorldPos = world;
    vViewPos = view.xyz;
    vFace = face;
    vBlock = block;
    vMaterial = material;
    vInfo = info;
}
