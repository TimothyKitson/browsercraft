#version 330 core

in vec2 vUV;
in vec4 vColor;
in vec3 vLight;

uniform sampler2D uTexture;
uniform sampler2D uNormalAtlas;
uniform int uIconNormals;   // 0 when the pack ships no normal maps

out vec4 FragColor;

void main()
{
    vec4 texel = texture(uTexture, vUV);
    vec4 colour = texel * vColor;

    // A block icon's faces carry the direction the light comes from, in
    // the face's own tangent space. Everything else on the HUD -- every
    // panel, slot and glyph -- passes zero and is drawn flat.
    //
    // This is the same relief the world gets, which is the point: a
    // cobblestone in the hotbar that looks nothing like the cobblestone
    // underfoot reads as two different blocks.
    if (uIconNormals == 1 && dot(vLight, vLight) > 0.0001)
    {
        vec4 normalSample = texture(uNormalAtlas, vUV);

        // LabPBR keeps X and Y; Z is what is left of a unit vector.
        vec2 tangentXY = normalSample.rg * 2.0 - 1.0;
        float tangentZ = sqrt(clamp(1.0 - dot(tangentXY, tangentXY), 0.0, 1.0));
        vec3 normal = normalize(vec3(tangentXY, tangentZ));

        vec3 toLight = normalize(vLight);

        // Wrapped rather than clamped: an icon is small, and a face
        // that went black in its hollows would read as a hole rather
        // than as texture.
        float lit = dot(normal, toLight) * 0.5 + 0.5;
        float level = toLight.z * 0.5 + 0.5;   // what a flat texel would get
        float shape = (lit - level) * 1.35;

        // And the height map's crevices, which owe nothing to where the
        // light is and so survive on the darkest of the three faces.
        float cavity = mix(0.72, 1.0, smoothstep(0.55, 1.0, normalSample.a));

        colour.rgb *= clamp(1.0 + shape, 0.45, 1.75) * cavity;
    }

    FragColor = colour;
    if (FragColor.a < 0.004) discard;
}
