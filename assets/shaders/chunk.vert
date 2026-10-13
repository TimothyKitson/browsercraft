#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in float aShade;      // face direction * ambient occlusion
layout (location = 3) in float aSkyLight;   // 0..1
layout (location = 4) in float aBlockLight; // 0..1
layout (location = 5) in float aFace;       // 0-5 cube faces, 6 = cross-shaped plant

uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uChunkOffset;

out vec2 vUV;
out float vShade;
out float vSkyLight;
out float vBlockLight;
out float vViewDistance;
out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vTangent;
out vec3 vBitangent;
flat out int vIsCross;
// 1 when the frame has to come from the triangle rather than from the
// table above: anything whose faces are not axis-aligned in the world,
// which is what the block in your hand is once the arm has turned it.
flat out int vDeriveBasis;

// Tangent frames per cube face, matching the UV layout the mesher emits:
// tangent follows +U, bitangent follows "up" in the texture image.
const vec3 FACE_NORMAL[6] = vec3[6](
    vec3( 1.0,  0.0,  0.0), vec3(-1.0,  0.0,  0.0),
    vec3( 0.0,  1.0,  0.0), vec3( 0.0, -1.0,  0.0),
    vec3( 0.0,  0.0,  1.0), vec3( 0.0,  0.0, -1.0)
);
const vec3 FACE_TANGENT[6] = vec3[6](
    vec3( 0.0,  0.0,  1.0), vec3( 0.0,  0.0,  1.0),
    vec3( 1.0,  0.0,  0.0), vec3( 1.0,  0.0,  0.0),
    vec3( 1.0,  0.0,  0.0), vec3( 1.0,  0.0,  0.0)
);
const vec3 FACE_BITANGENT[6] = vec3[6](
    vec3( 0.0,  1.0,  0.0), vec3( 0.0,  1.0,  0.0),
    vec3( 0.0,  0.0, -1.0), vec3( 0.0,  0.0, -1.0),
    vec3( 0.0,  1.0,  0.0), vec3( 0.0,  1.0,  0.0)
);

void main()
{
    vec3 worldPos = aPos + uChunkOffset;
    vec4 viewPos = uView * vec4(worldPos, 1.0);

    gl_Position = uProjection * viewPos;

    vUV = aUV;
    vShade = aShade;
    vSkyLight = aSkyLight;
    vBlockLight = aBlockLight;
    vViewDistance = length(viewPos.xyz);
    vWorldPos = worldPos;

    int face = int(aFace + 0.5);
    if (face == 7)
    {
        // Mapped, but turned: the fragment stage works the frame out
        // from the triangle it is shading.
        vIsCross = 0;
        vDeriveBasis = 1;
        vNormal = vec3(0.0, 1.0, 0.0);
        vTangent = vec3(1.0, 0.0, 0.0);
        vBitangent = vec3(0.0, 0.0, -1.0);
    }
    else if (face >= 6)
    {
        // Plants are billboards: just face upwards and skip normal mapping.
        vIsCross = 1;
        vDeriveBasis = 0;
        vNormal = vec3(0.0, 1.0, 0.0);
        vTangent = vec3(1.0, 0.0, 0.0);
        vBitangent = vec3(0.0, 0.0, -1.0);
    }
    else
    {
        vIsCross = 0;
        vDeriveBasis = 0;
        vNormal = FACE_NORMAL[face];
        vTangent = FACE_TANGENT[face];
        vBitangent = FACE_BITANGENT[face];
    }
}
