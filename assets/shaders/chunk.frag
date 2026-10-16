#version 330 core

in vec2 vUV;
in float vShade;
in float vSkyLight;
in float vBlockLight;
in float vViewDistance;
in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vTangent;
in vec3 vBitangent;
flat in int vIsCross;
flat in int vDeriveBasis;

uniform sampler2D uAtlas;
uniform sampler2D uNormalAtlas;   // LabPBR _n: normal.xy, ambient occlusion, height
uniform sampler2D uSpecularAtlas; // LabPBR _s: smoothness, F0, porosity, emission

uniform float uDaylight;     // 0 at midnight, 1 at noon
uniform vec3 uFogColor;
uniform float uFogStart;
uniform float uFogEnd;
uniform int uUnderwater;
uniform vec3 uSunDirection;
uniform vec3 uCameraPos;
uniform int uPbrEnabled;
// Light floor for a world with no sun. The Nether is lit well above
// pitch black even where nothing is burning.
uniform float uAmbient;

out vec4 FragColor;

// The atlas is a grid of tiles, so a UV may only be moved within the
// tile it landed in. Step outside and the stone picks up the edge of
// whatever was painted next to it.
const float ATLAS_TILES = 16.0;

// How deep the relief reads, as a fraction of a block. Small: this is
// the one knob that turns believable stonework into melted wax.
const float PARALLAX_DEPTH = 0.075;

// Walks the view ray into the surface until it meets the height map,
// and gives back the UV of the point it actually hit. This is what
// makes a wall look cut rather than painted: the texels move against
// each other as you walk past, which no amount of shading can fake.
vec2 parallaxUV(vec2 uv, vec3 viewTangent)
{
    vec2 tile = floor(uv * ATLAS_TILES);
    vec2 within = uv * ATLAS_TILES - tile;

    // More steps edge-on, where the ray crosses more of the height
    // field and a coarse march shows up as banding.
    float layers = mix(28.0, 10.0, clamp(abs(viewTangent.z), 0.0, 1.0));
    float layerDepth = 1.0 / layers;

    // The ray's travel across the surface per layer of depth. Clamped
    // so a ray that is almost edge-on does not shoot off sideways.
    vec2 step = (viewTangent.xy / max(abs(viewTangent.z), 0.35))
                * PARALLAX_DEPTH / layers;

    float depth = 0.0;
    vec2 at = within;
    float surface = 1.0 - textureLod(uNormalAtlas, (tile + fract(at)) / ATLAS_TILES, 0.0).a;

    for (int i = 0; i < 32; ++i)
    {
        if (depth >= surface) break;
        at -= step;
        surface = 1.0 - textureLod(uNormalAtlas, (tile + fract(at)) / ATLAS_TILES, 0.0).a;
        depth += layerDepth;
    }

    // One step back, and split the difference between the last sample
    // above the surface and the first below it. Without this the march
    // shows as steps across a slope.
    vec2 before = at + step;
    float after = surface - depth;
    float priorSurface = 1.0 - textureLod(uNormalAtlas,
                                          (tile + fract(before)) / ATLAS_TILES, 0.0).a;
    float prior = priorSurface - depth + layerDepth;

    float blend = after / max(after - prior, 0.0001);
    at = mix(at, before, clamp(blend, 0.0, 1.0));

    return (tile + fract(at)) / ATLAS_TILES;
}

void main()
{
    vec2 uv = vUV;

    // The frame this fragment is shaded in. Usually the one the vertex
    // stage looked up, but a surface that has been turned -- the block
    // in your hand -- has to have it worked out from the triangle, or
    // the relief would be lit as though the block were still sitting
    // in the ground where its texture came from.
    vec3 faceNormal = vNormal;
    vec3 faceTangent = vTangent;
    vec3 faceBitangent = vBitangent;

    if (vDeriveBasis == 1)
    {
        vec3 alongX = dFdx(vWorldPos);
        vec3 alongY = dFdy(vWorldPos);
        vec2 uvX = dFdx(vUV);
        vec2 uvY = dFdy(vUV);

        faceNormal = normalize(cross(alongX, alongY));

        // Winding decides which way that cross product points, and the
        // held item is wound backwards on purpose. Turn it to face the
        // eye rather than trusting it.
        if (dot(faceNormal, uCameraPos - vWorldPos) < 0.0) faceNormal = -faceNormal;

        vec3 acrossY = cross(alongY, faceNormal);
        vec3 acrossX = cross(faceNormal, alongX);
        faceTangent = acrossY * uvX.x + acrossX * uvY.x;
        faceBitangent = acrossY * uvX.y + acrossX * uvY.y;

        float longest = inversesqrt(max(dot(faceTangent, faceTangent),
                                        dot(faceBitangent, faceBitangent)));
        faceTangent *= longest;
        faceBitangent *= longest;
    }

    // The relief has to be found before anything is sampled, since
    // every map is then read at the point the eye actually sees.
    if (uPbrEnabled == 1 && vIsCross == 0)
    {
        vec3 toEye = normalize(uCameraPos - vWorldPos);
        vec3 viewTangent = vec3(dot(toEye, faceTangent),
                                dot(toEye, faceBitangent),
                                dot(toEye, faceNormal));
        uv = parallaxUV(vUV, viewTangent);
    }

    vec4 texel = texture(uAtlas, uv);
    if (texel.a < 0.1) discard; // leaf gaps, plant cut-outs

    // Baked voxel lighting: sunlight dims at night, torch light never does.
    float sky = vSkyLight * uDaylight;
    float level = max(sky, vBlockLight);
    float brightness = mix(uAmbient, 1.0, level);

    vec3 albedo = texel.rgb;
    vec3 color = albedo * brightness * vShade;

    if (uPbrEnabled == 1 && vIsCross == 0)
    {
        vec4 normalSample = texture(uNormalAtlas, uv);
        vec4 specularSample = texture(uSpecularAtlas, uv);

        // LabPBR stores only X and Y; Z is reconstructed as a unit vector.
        vec2 tangentXY = normalSample.rg * 2.0 - 1.0;
        float tangentZ = sqrt(clamp(1.0 - dot(tangentXY, tangentXY), 0.0, 1.0));
        vec3 normal = normalize(faceTangent * tangentXY.x +
                                faceBitangent * tangentXY.y +
                                faceNormal * tangentZ);

        float materialAO = normalSample.b;
        float height = normalSample.a;
        float smoothness = specularSample.r;
        float reflectance = specularSample.g;
        // Alpha 255 means "not emissive"; 0-254 maps to 0..1.
        float emission = specularSample.a > 0.996 ? 0.0 : specularSample.a * (255.0 / 254.0);

        // Only surfaces the sky actually reaches get direct sun.
        float sunReach = vSkyLight * uDaylight;
        float incidence = max(dot(normal, uSunDirection), 0.0);
        float direct = incidence * sunReach;

        // Shape, measured as the texel's departure from the flat face so
        // the baked per-face shading is not counted twice. The halved
        // form this used -- dot * 0.5 + 0.5 on both sides -- threw away
        // half of an already small number, and a wall with a perfectly
        // good normal map came out looking painted on. The difference
        // of the two dots is taken whole, which is the actual Lambert
        // term the bump earns over the flat face.
        float bumpLit = dot(normal, uSunDirection);
        float flatLit = dot(faceNormal, uSunDirection);
        // Gained down from 1.45. Taking the Lambert difference whole
        // was the fix for a wall that looked painted on, but on a
        // texture whose normals run to half a unit -- cobblestone, and
        // the stone bricks -- it swung neighbouring texels through
        // three times the brightness and the stone came out as a
        // jumble rather than as stonework. Readable relief, not noise.
        float shape = (bumpLit - flatLit) * mix(0.42, 0.72, sunReach);

        // The pack's height map darkens the crevices. This one owes
        // nothing to the sun, so it survives shade, night and caves.
        // Read across the whole range rather than only the top half:
        // the atlas has already stretched each tile to fill it.
        // Narrowed to match: with each tile's height now stretched to
        // the full range, the old span darkened a crevice by two fifths
        // on its own, and that landed on top of the shape term.
        float cavity = mix(0.80, 1.04, height);

        // The clamp is the backstop rather than the shape of the
        // effect: anything reaching it is a texel the normal map has
        // turned nearly away from the sun, and a five-fold swing
        // between one cobble and the next is not what that should look
        // like.
        float relief = mix(0.90, 1.12, direct) * clamp(1.0 + shape, 0.66, 1.34);
        color = albedo * brightness * vShade * relief * cavity * mix(1.0, materialAO, 0.75);

        // Blinn-Phong highlight, sharpened by the pack's smoothness map.
        vec3 viewDir = normalize(uCameraPos - vWorldPos);
        vec3 halfway = normalize(viewDir + uSunDirection);
        float specularPower = exp2(1.0 + smoothness * 9.0);
        float highlight = pow(max(dot(normal, halfway), 0.0), specularPower);
        color += vec3(1.0, 0.97, 0.90) * highlight * smoothness * (0.04 + reflectance) * direct * 2.0;

        color += albedo * emission * 1.6;
    }

    // A world with no sun still has to show its surfaces. Flooring the
    // light level alone was not enough: per-face shading and the PBR
    // relief term multiply on top of it and drag everything back to
    // black, so the floor is applied to the finished colour.
    color = max(color, albedo * uAmbient);

    float fogAmount = clamp((vViewDistance - uFogStart) / max(uFogEnd - uFogStart, 0.001), 0.0, 1.0);
    vec3 fogColor = uFogColor;

    if (uUnderwater == 1)
    {
        color = mix(color, vec3(0.10, 0.30, 0.55), 0.45);
        fogColor = vec3(0.07, 0.21, 0.42);
        fogAmount = clamp(vViewDistance / 24.0, 0.0, 1.0);
    }

    color = mix(color, fogColor, fogAmount);
    FragColor = vec4(color, texel.a);
}
