#!/usr/bin/env python3
"""Check Browsercraft's built-in GLSL files and foliage-wind wiring.

This checks the file graph and the engine contracts. If glslangValidator is
installed, it also compiles every shader stage after expanding the same
one-level includes used by Shader::readFile.
"""
from __future__ import annotations

import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SHADER_ROOT = ROOT / "assets" / "shaders"
FX_ROOT = SHADER_ROOT / "fx"

PROGRAMS = (
    ("chunk.vert", "chunk.frag"),
    ("shadow.vert", "shadow.frag"),
    ("world.vert", "world.frag"),
    ("screen.vert", "sky.frag"),
    ("screen.vert", "composite.frag"),
    ("screen.vert", "bloom_down.frag"),
    ("screen.vert", "bloom_up.frag"),
    ("screen.vert", "grade.frag"),
    ("screen.vert", "smooth.frag"),
)

INCLUDE_RE = re.compile(r'^\s*#include\s+"([^"]+)"\s*$')


def fail(message: str) -> None:
    print(f"FAIL: {message}", file=sys.stderr)


def expand_includes(path: Path, errors: list[str]) -> str:
    if not path.is_file():
        errors.append(f"missing shader file: {path.relative_to(ROOT)}")
        return ""

    source = path.read_text(encoding="utf-8")
    expanded: list[str] = []
    for number, line in enumerate(source.splitlines(), 1):
        match = INCLUDE_RE.match(line)
        if not match:
            if "#include" in line:
                errors.append(
                    f"unsupported include syntax at {path.relative_to(ROOT)}:{number}"
                )
            expanded.append(line)
            continue

        include_path = (path.parent / match.group(1)).resolve()
        try:
            include_path.relative_to(ROOT)
        except ValueError:
            errors.append(
                f"include escapes repository at {path.relative_to(ROOT)}:{number}"
            )
            continue

        if not include_path.is_file():
            errors.append(
                f"missing include {match.group(1)} from {path.relative_to(ROOT)}:{number}"
            )
            continue

        include_source = include_path.read_text(encoding="utf-8")
        if any(INCLUDE_RE.match(row) for row in include_source.splitlines()):
            errors.append(
                f"nested include unsupported by Shader::readFile: "
                f"{include_path.relative_to(ROOT)}"
            )
        expanded.append(include_source)

    result = "\n".join(expanded) + "\n"
    first = next((row.strip() for row in result.splitlines() if row.strip()), "")
    if first != "#version 330 core":
        errors.append(
            f"{path.relative_to(ROOT)} must begin with #version 330 core"
        )
    return result


def main() -> int:
    errors: list[str] = []
    expanded_sources: list[tuple[str, str]] = []

    for vertex_name, fragment_name in PROGRAMS:
        for name, stage in ((vertex_name, "vert"), (fragment_name, "frag")):
            # The fx shaders live in fx/; the base chunk shaders live one level up.
            path = (FX_ROOT if name not in ("chunk.vert", "chunk.frag") else SHADER_ROOT) / name
            source = expand_includes(path, errors)
            expanded_sources.append((stage, source))

    pipeline_cpp = ROOT / "src" / "Renderer" / "ShaderPipeline.cpp"
    shader_cpp = ROOT / "src" / "Renderer" / "Shader.cpp"
    mesher_cpp = ROOT / "src" / "World" / "ChunkMesher.cpp"
    common_path = FX_ROOT / "common.glsl"
    world_vert = FX_ROOT / "world.vert"
    shadow_vert = FX_ROOT / "shadow.vert"

    for path in (pipeline_cpp, shader_cpp, mesher_cpp, common_path, world_vert, shadow_vert):
        if not path.is_file():
            errors.append(f"missing engine shader dependency: {path.relative_to(ROOT)}")

    if not errors:
        pipeline = pipeline_cpp.read_text(encoding="utf-8")
        shader_loader = shader_cpp.read_text(encoding="utf-8")
        mesher = mesher_cpp.read_text(encoding="utf-8")
        common = common_path.read_text(encoding="utf-8")
        world = world_vert.read_text(encoding="utf-8")
        shadow = shadow_vert.read_text(encoding="utf-8")

        # Keep the declared ShaderPipeline programs and the files in sync.
        for vertex_name, fragment_name in PROGRAMS[1:]:
            for name in (vertex_name, fragment_name):
                if name != "screen.vert" and f'dir + "{name}"' not in pipeline:
                    errors.append(f"ShaderPipeline no longer loads fx/{name}")

        # Browser builds must translate desktop GLSL to WebGL2's GLSL ES.
        if "#version 300 es" not in shader_loader or "adaptForPlatform" not in shader_loader:
            errors.append("Shader.cpp is missing its WebGL2 GLSL ES adaptation")

        # Wind must deform both the visible mesh and its shadow casters.
        if "vec3 waveOffset(" not in common:
            errors.append("common.glsl is missing waveOffset")
        if "waveOffset(" not in world or "waveOffset(" not in shadow:
            errors.append("world and shadow vertex shaders must share waveOffset")
        if 'setFloat("uTime", m_frame.time)' not in pipeline:
            errors.append("the shader pipeline is not passing animation time")
        if "tagOf(CROSS_FACE, id, true)" not in mesher:
            errors.append("plant top vertices are no longer tagged for wind deformation")

        for material in (
            "MAT_PLANT",
            "MAT_LEAVES",
            "MAT_HANGING",
            "MAT_UNDERWATER_PLANT",
            "MAT_FLOATING",
        ):
            if material not in common:
                errors.append(f"common.glsl is missing foliage/water material {material}")

    if errors:
        for error in errors:
            fail(error)
        return 1

    print(f"PASS: checked {len(PROGRAMS)} shader programs and their includes.")
    print("PASS: Browsercraft shader toggle pipeline and foliage wind wiring are present.")

    compiler = shutil.which("glslangValidator")
    if not compiler:
        print("NOTE: glslangValidator not installed; GLSL compiler checks were skipped.")
        return 0

    with tempfile.TemporaryDirectory(prefix="browsercraft-shaders-") as temp_dir:
        for index, (stage, source) in enumerate(expanded_sources):
            source_path = Path(temp_dir) / f"shader_{index}.{stage}"
            source_path.write_text(source, encoding="utf-8")
            result = subprocess.run(
                [compiler, "-S", stage, str(source_path)],
                capture_output=True,
                text=True,
                check=False,
            )
            if result.returncode:
                fail(f"{source_path.name} did not compile:\n{result.stdout}{result.stderr}")
                return 1

    print(f"PASS: glslangValidator compiled {len(expanded_sources)} shader stages.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
