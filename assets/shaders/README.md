# Browsercraft shader pipeline

Open **Options → Video Settings → Shaders** to enable the built-in shader
pipeline. It is implemented for this engine's OpenGL renderer and uses the
programs in `fx/` for shadows, the world, sky, reflections, bloom, and colour
grading. Web builds adapt the same GLSL 3.30 source to WebGL 2 / GLSL ES 3.00.

## Vegetation movement

With Shaders enabled, the vertex shaders apply a slow procedural breeze:

- The upper vertices of crossed grass and other small plants move while the
  base vertices stay anchored.
- Leaf blocks move subtly, and vines / hanging plants have a gentler sway.
- Kelp, seagrass, and floating plants move with their own softer motion.
- Water surfaces have a separate small swell.

The visible world pass and shadow pass both call the same `waveOffset`
function, so moving plants keep their shadows in step.

## Shader-pack compatibility

This directory is **not** an OptiFine / Iris shader-pack loader. Minecraft
shader packs use Minecraft-specific passes, uniforms, and render targets
(for example `gbuffers_*`, `deferred*`, and `composite*`). Browsercraft has
its own vertex layout, framebuffers, and shader uniforms, so those files
cannot be copied here and expected to work. A compatible port would need to
be written for this engine's renderer.

The uploaded Complementary Development archive is therefore not copied into
this public repository. Its included agreement also restricts redistribution
and modification. The built-in shaders in `fx/` remain the game's active,
compatible shader implementation.

## Check the shader files

Run:

```sh
python tools/check-shaders.py
```

The check verifies every shader file used by the pipeline, include paths,
WebGL 2 adaptation, and the shared foliage-wind path. When
`glslangValidator` is installed, it also compiles each expanded shader
stage. The actual OpenGL program link still happens when the game starts
and the **Shaders** option is enabled.
