import math
import os
import sys

import bpy
import numpy as np
from mathutils import Vector, Matrix

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rigs

FPS = 30
SHOT_NAMES = ['Overworld', 'Nether Bastion', 'Nether Fortress', 'The End']


def engine_to_blender(arr):
    out = np.empty_like(arr)
    out[:, 0] = arr[:, 0]
    out[:, 1] = -arr[:, 2]
    out[:, 2] = arr[:, 1]
    return out


def make_mesh(name, co, uv, colors):
    n = len(co)
    me = bpy.data.meshes.new(name)
    me.vertices.add(n)
    me.vertices.foreach_set('co', co.astype(np.float32).ravel())
    me.loops.add(n)
    me.loops.foreach_set('vertex_index', np.arange(n, dtype=np.int32))
    me.polygons.add(n // 3)
    me.polygons.foreach_set('loop_start', np.arange(0, n, 3, dtype=np.int32))
    me.update(calc_edges=True)
    layer = me.uv_layers.new(name='UVMap')
    layer.data.foreach_set('uv', uv.astype(np.float32).ravel())
    for cname, c in colors.items():
        attr = me.color_attributes.new(cname, 'FLOAT_COLOR', 'CORNER')
        attr.data.foreach_set('color', c.astype(np.float32).ravel())
    return me


def block_material(name, atlas, blended=False):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    nodes = nt.nodes
    links = nt.links
    nodes.clear()
    out = nodes.new('ShaderNodeOutputMaterial')
    tex = nodes.new('ShaderNodeTexImage')
    tex.image = atlas
    tex.interpolation = 'Closest'
    light = nodes.new('ShaderNodeVertexColor')
    light.layer_name = 'Light'
    tint = nodes.new('ShaderNodeVertexColor')
    tint.layer_name = 'Tint'
    split = nodes.new('ShaderNodeSeparateColor')
    links.new(tex.outputs['Color'], split.inputs['Color'])
    greener = nodes.new('ShaderNodeMath')
    greener.operation = 'SUBTRACT'
    links.new(split.outputs['Green'], greener.inputs[0])
    links.new(split.outputs['Red'], greener.inputs[1])
    mask = nodes.new('ShaderNodeMath')
    mask.operation = 'GREATER_THAN'
    links.new(greener.outputs[0], mask.inputs[0])
    mask.inputs[1].default_value = 0.02
    keep = nodes.new('ShaderNodeMath')
    keep.operation = 'SUBTRACT'
    keep.inputs[0].default_value = 1.0
    links.new(mask.outputs[0], keep.inputs[1])
    weight = nodes.new('ShaderNodeMath')
    weight.operation = 'MULTIPLY'
    links.new(tint.outputs['Alpha'], weight.inputs[0])
    links.new(keep.outputs[0], weight.inputs[1])
    gap = nodes.new('ShaderNodeVectorMath')
    gap.operation = 'SUBTRACT'
    gap.inputs[0].default_value = (1.0, 1.0, 1.0)
    links.new(tint.outputs['Color'], gap.inputs[1])
    scaled = nodes.new('ShaderNodeVectorMath')
    scaled.operation = 'SCALE'
    links.new(gap.outputs['Vector'], scaled.inputs[0])
    links.new(weight.outputs[0], scaled.inputs['Scale'])
    effective = nodes.new('ShaderNodeVectorMath')
    effective.operation = 'ADD'
    links.new(tint.outputs['Color'], effective.inputs[0])
    links.new(scaled.outputs['Vector'], effective.inputs[1])
    tinted = nodes.new('ShaderNodeVectorMath')
    tinted.operation = 'MULTIPLY'
    links.new(tex.outputs['Color'], tinted.inputs[0])
    links.new(effective.outputs['Vector'], tinted.inputs[1])
    lit = nodes.new('ShaderNodeVectorMath')
    lit.operation = 'MULTIPLY'
    links.new(tinted.outputs['Vector'], lit.inputs[0])
    links.new(light.outputs['Color'], lit.inputs[1])
    emit = nodes.new('ShaderNodeEmission')
    links.new(lit.outputs['Vector'], emit.inputs['Color'])
    see = nodes.new('ShaderNodeBsdfTransparent')
    mix = nodes.new('ShaderNodeMixShader')
    if blended:
        cover = nodes.new('ShaderNodeMath')
        cover.operation = 'MULTIPLY'
        links.new(tex.outputs['Alpha'], cover.inputs[0])
        cover.inputs[1].default_value = 0.9
        links.new(cover.outputs[0], mix.inputs['Fac'])
    else:
        cut = nodes.new('ShaderNodeMath')
        cut.operation = 'GREATER_THAN'
        links.new(tex.outputs['Alpha'], cut.inputs[0])
        cut.inputs[1].default_value = 0.1
        links.new(cut.outputs[0], mix.inputs['Fac'])
    links.new(see.outputs['BSDF'], mix.inputs[1])
    links.new(emit.outputs['Emission'], mix.inputs[2])
    links.new(mix.outputs['Shader'], out.inputs['Surface'])
    if hasattr(mat, 'surface_render_method'):
        mat.surface_render_method = 'BLENDED' if blended else 'DITHERED'
    mat.use_backface_culling = False
    return mat


def mob_material(name, img):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    out = nt.nodes.new('ShaderNodeOutputMaterial')
    tex = nt.nodes.new('ShaderNodeTexImage')
    tex.image = img
    tex.interpolation = 'Closest'
    shade = nt.nodes.new('ShaderNodeVertexColor')
    shade.layer_name = 'Light'
    lit = nt.nodes.new('ShaderNodeVectorMath')
    lit.operation = 'MULTIPLY'
    nt.links.new(tex.outputs['Color'], lit.inputs[0])
    nt.links.new(shade.outputs['Color'], lit.inputs[1])
    emit = nt.nodes.new('ShaderNodeEmission')
    nt.links.new(lit.outputs['Vector'], emit.inputs['Color'])
    see = nt.nodes.new('ShaderNodeBsdfTransparent')
    cut = nt.nodes.new('ShaderNodeMath')
    cut.operation = 'GREATER_THAN'
    nt.links.new(tex.outputs['Alpha'], cut.inputs[0])
    cut.inputs[1].default_value = 0.1
    mix = nt.nodes.new('ShaderNodeMixShader')
    nt.links.new(cut.outputs[0], mix.inputs['Fac'])
    nt.links.new(see.outputs['BSDF'], mix.inputs[1])
    nt.links.new(emit.outputs['Emission'], mix.inputs[2])
    nt.links.new(mix.outputs['Shader'], out.inputs['Surface'])
    if hasattr(mat, 'surface_render_method'):
        mat.surface_render_method = 'DITHERED'
    mat.use_backface_culling = False
    return mat


def load_terrain(path, name, mat, collection, floor=None):
    if not os.path.exists(path) or os.path.getsize(path) == 0:
        return None
    data = np.fromfile(path, dtype=np.float32).reshape(-1, 10)
    data = data[: (len(data) // 3) * 3]
    if floor is not None:
        tops = data[:, 1].reshape(-1, 3).max(axis=1)
        keep = np.repeat(tops >= floor, 3)
        data = data[keep]
    if len(data) == 0:
        return None
    co = engine_to_blender(data[:, 0:3])
    uv = np.stack([data[:, 3], 1.0 - data[:, 4]], axis=1)
    light = np.repeat(data[:, 5:6], 3, axis=1)
    light = np.concatenate([light, np.ones((len(data), 1), np.float32)], axis=1)
    tint = data[:, 6:10]
    me = make_mesh(name, co, uv, {'Light': light, 'Tint': tint})
    me.materials.append(mat)
    obj = bpy.data.objects.new(name, me)
    collection.objects.link(obj)
    return obj


def read_mobs(folder):
    path = os.path.join(folder, 'mobs.bin')
    names = {}
    listing = os.path.join(folder, 'mobs.txt')
    if os.path.exists(listing):
        for line in open(listing):
            parts = line.strip().split(' ', 1)
            if len(parts) == 2:
                names[int(parts[0])] = parts[1]
    layers = []
    if not os.path.exists(path):
        return layers, names
    raw = open(path, 'rb').read()
    at = 0
    while at + 12 <= len(raw):
        serial, count = np.frombuffer(raw[at:at + 8], dtype=np.uint32)
        length = int(np.frombuffer(raw[at + 8:at + 12], dtype=np.uint32)[0])
        at += 12
        texture = raw[at:at + length].decode()
        at += length
        verts = np.frombuffer(raw[at:at + int(count) * 24], dtype=np.float32).reshape(-1, 6)
        at += int(count) * 24
        layers.append((int(serial), texture, verts))
    return layers, names


def read_track(folder):
    cameras = {}
    mobs = {}
    for line in open(os.path.join(folder, 'track.txt')):
        p = line.split()
        if p[0] == 'camera':
            cameras[int(p[1])] = (Vector((float(p[2]), -float(p[4]), float(p[3]))), Vector((float(p[5]), -float(p[7]), float(p[6]))))
        elif p[0] == 'mob':
            mobs.setdefault(int(p[2]), {})[int(p[1])] = (Vector((float(p[3]), -float(p[5]), float(p[4]))), float(p[6]))
    return cameras, mobs


def read_info(folder):
    info = {'crystal': []}
    for line in open(os.path.join(folder, 'shot.txt')):
        p = line.split()
        if p[0] == 'crystal':
            info['crystal'].append([int(x) for x in p[1:]])
        else:
            info[p[0]] = [float(x) for x in p[1:]]
    return info


ENGINE_TO_BLENDER = Matrix(((1.0, 0.0, 0.0, 0.0), (0.0, 0.0, -1.0, 0.0), (0.0, 1.0, 0.0, 0.0), (0.0, 0.0, 0.0, 1.0)))


def flat_material(name, img):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    out = nt.nodes.new('ShaderNodeOutputMaterial')
    tex = nt.nodes.new('ShaderNodeTexImage')
    tex.image = img
    tex.interpolation = 'Closest'
    emit = nt.nodes.new('ShaderNodeEmission')
    nt.links.new(tex.outputs['Color'], emit.inputs['Color'])
    see = nt.nodes.new('ShaderNodeBsdfTransparent')
    cut = nt.nodes.new('ShaderNodeMath')
    cut.operation = 'GREATER_THAN'
    nt.links.new(tex.outputs['Alpha'], cut.inputs[0])
    cut.inputs[1].default_value = 0.1
    mix = nt.nodes.new('ShaderNodeMixShader')
    nt.links.new(cut.outputs[0], mix.inputs['Fac'])
    nt.links.new(see.outputs['BSDF'], mix.inputs[1])
    nt.links.new(emit.outputs['Emission'], mix.inputs[2])
    nt.links.new(mix.outputs['Shader'], out.inputs['Surface'])
    if hasattr(mat, 'surface_render_method'):
        mat.surface_render_method = 'DITHERED'
    mat.use_backface_culling = False
    return mat


def cube_object(name, origin, size, uv, mat, collection):
    verts = []
    faces = []
    uvs = []
    for corners, corner_uv in rigs.cuboid_quads(origin, size, uv, (64, 32)):
        start = len(verts)
        verts.extend([(c[0] / 16.0, c[1] / 16.0, c[2] / 16.0) for c in corners])
        faces.append(tuple(range(start, start + 4)))
        uvs.extend(corner_uv)
    me = bpy.data.meshes.new(name)
    me.from_pydata(verts, [], faces)
    layer = me.uv_layers.new(name='UVMap')
    for i, loop in enumerate(me.loops):
        layer.data[i].uv = uvs[loop.vertex_index]
    me.materials.append(mat)
    obj = bpy.data.objects.new(name, me)
    collection.objects.link(obj)
    obj.rotation_mode = 'QUATERNION'
    return obj


def crystal_age(x, y, z):
    seed = ((x * 73856093) & 0xFFFFFFFF) ^ ((y * 19349663) & 0xFFFFFFFF) ^ ((z * 83492791) & 0xFFFFFFFF)
    seed = ((seed ^ (seed >> 15)) * 2246822519) & 0xFFFFFFFF
    return float(seed % 100000)


def build_crystals(info, frames, assets, collection):
    if not info['crystal']:
        return
    img = bpy.data.images.load(os.path.join(assets, 'textures/entity/end_crystal/end_crystal.png'), check_existing=True)
    mat = flat_material('End crystal', img)
    start = info.get('ticks', [0.0])[0]
    s45 = math.sin(math.pi / 4.0)
    diagonal = Matrix.Rotation(math.pi / 3.0, 4, Vector((s45, 0.0, s45)).normalized())
    for k, (x, y, z) in enumerate(info['crystal']):
        group = bpy.data.collections.new('End Crystal %d' % (k + 1))
        collection.children.link(group)
        base = cube_object('Crystal %d base' % (k + 1), (-6, 0, -6), (12, 4, 12), (0, 16), mat, group)
        outer = cube_object('Crystal %d glass outer' % (k + 1), (-4, -4, -4), (8, 8, 8), (0, 0), mat, group)
        inner = cube_object('Crystal %d glass inner' % (k + 1), (-4, -4, -4), (8, 8, 8), (0, 0), mat, group)
        core = cube_object('Crystal %d core' % (k + 1), (-4, -4, -4), (8, 8, 8), (32, 0), mat, group)
        age0 = crystal_age(x, y, z) + start
        for frame in range(1, frames + 1, 2):
            age = age0 + (frame - 1) * 20.0 / FPS
            g = math.sin(age * 0.2) / 2.0 + 0.5
            h = (g * g + g) * 0.4 - 1.4
            spin = Matrix.Rotation(math.radians(age * 3.0), 4, 'Y')
            m = Matrix.Translation((x + 0.5, y, z + 0.5)) @ Matrix.Scale(2.0, 4) @ Matrix.Translation((0.0, -0.5, 0.0))
            parts = [(base, m)]
            m = m @ spin @ Matrix.Translation((0.0, 1.5 + h / 2.0, 0.0)) @ diagonal
            parts.append((outer, m))
            m = m @ Matrix.Scale(0.875, 4) @ diagonal @ spin
            parts.append((inner, m))
            m = m @ Matrix.Scale(0.875, 4) @ diagonal @ spin
            parts.append((core, m))
            for obj, local in parts:
                obj.matrix_world = ENGINE_TO_BLENDER @ local
                obj.keyframe_insert('location', frame=frame)
                obj.keyframe_insert('rotation_quaternion', frame=frame)
                obj.keyframe_insert('scale', frame=frame)


def heading(track, frame, fallback):
    frames = sorted(track)
    before = [f for f in frames if f <= frame]
    after = [f for f in frames if f > frame]
    if before and after:
        delta = track[after[0]][0] - track[before[-1]][0]
        if delta.xy.length > 0.02:
            return math.atan2(delta.y, delta.x)
    return fallback


def build_shot(index, folder, atlas, export_dir, assets):
    info = read_info(folder)
    frames = int(info['frames'][0])
    scene = bpy.context.scene
    scene.name = 'Shot %d - %s' % (index + 1, SHOT_NAMES[index])
    scene.render.fps = FPS
    scene.frame_start = 1
    scene.frame_end = frames
    scene.render.resolution_x = 1920
    scene.render.resolution_y = 1080
    world = bpy.data.worlds.new(scene.name + ' Sky')
    world.use_nodes = True
    sky = info.get('sky', [0.5, 0.7, 1.0])
    if index == 0:
        sky = [0.47, 0.65, 1.0]
    nt = world.node_tree
    back = nt.nodes['Background']
    back.inputs['Color'].default_value = (sky[0], sky[1], sky[2], 1.0)
    fill = nt.nodes.new('ShaderNodeBackground')
    fill.inputs['Color'].default_value = (1.0, 1.0, 1.0, 1.0)
    fill.inputs['Strength'].default_value = 0.6 if index == 0 else 0.35
    path = nt.nodes.new('ShaderNodeLightPath')
    pick = nt.nodes.new('ShaderNodeMixShader')
    nt.links.new(path.outputs['Is Camera Ray'], pick.inputs['Fac'])
    nt.links.new(fill.outputs['Background'], pick.inputs[1])
    nt.links.new(back.outputs['Background'], pick.inputs[2])
    nt.links.new(pick.outputs['Shader'], nt.nodes['World Output'].inputs['Surface'])
    scene.world = world
    root = bpy.data.collections.new(scene.name + ' World')
    scene.collection.children.link(root)
    cameras, tracks = read_track(folder)
    lowest = min(c[0].z for c in cameras.values()) if cameras else 0.0
    floor = lowest - 48.0 if index in (0, 3) else None
    load_terrain(os.path.join(folder, 'terrain.bin'), 'Terrain', block_material('Blocks', atlas), root, floor)
    load_terrain(os.path.join(folder, 'terrain_alpha.bin'), 'Water and Glass', block_material('Blocks (translucent)', atlas, True), root, floor)
    build_crystals(info, frames, assets, root)

    layers, names = read_mobs(folder)
    mob_collection = bpy.data.collections.new(scene.name + ' Mobs')
    scene.collection.children.link(mob_collection)
    materials = {}
    grouped = {}
    for serial, texture, verts in layers:
        grouped.setdefault(serial, []).append((texture, verts))
    for serial, parts in grouped.items():
        kind = names.get(serial, 'Mob')
        if kind in ('Ender Dragon', 'Blaze'):
            continue
        objs = []
        for texture, verts in parts:
            if texture not in materials:
                materials[texture] = mob_material(texture, bpy.data.images.load(os.path.join(export_dir, texture), check_existing=True))
            co = engine_to_blender(verts[:, 0:3])
            uv = np.stack([verts[:, 3], 1.0 - verts[:, 4]], axis=1)
            shade = np.repeat(verts[:, 5:6], 3, axis=1)
            shade = np.concatenate([shade, np.ones((len(verts), 1), np.float32)], axis=1)
            me = make_mesh('%s %d' % (kind, serial), co, uv, {'Light': shade})
            me.materials.append(materials[texture])
            obj = bpy.data.objects.new('%s %d' % (kind, serial), me)
            mob_collection.objects.link(obj)
            objs.append(obj)
        path = tracks.get(serial, {})
        if not path:
            continue
        start = min(path)
        base_yaw = path[start][1]
        holder = bpy.data.objects.new('%s %d (path)' % (kind, serial), None)
        mob_collection.objects.link(holder)
        for obj in objs:
            obj.parent = holder
        for frame, (pos, yaw) in sorted(path.items()):
            holder.location = pos
            holder.rotation_euler = (0.0, 0.0, -math.radians(yaw - base_yaw))
            holder.keyframe_insert('location', frame=frame + 1)
            holder.keyframe_insert('rotation_euler', frame=frame + 1)

    for serial, kind in names.items():
        if kind not in ('Ender Dragon', 'Blaze'):
            continue
        path = tracks.get(serial, {})
        if not path:
            continue
        if kind == 'Ender Dragon':
            rig = rigs.build_dragon(mob_collection, assets, 'Ender Dragon')
            rigs.animate_dragon(rig, 1, frames)
            facing = -math.pi / 2.0
        else:
            rig = rigs.build_blaze(mob_collection, assets, 'Blaze %d' % serial)
            rigs.animate_blaze(rig, 1, frames, start_ticks=serial * 7.0)
            facing = -math.pi / 2.0
        arm = rig.armature
        arm.rotation_mode = 'XYZ'
        last = None
        for frame in sorted(path):
            pos = path[frame][0]
            arm.location = pos
            angle = heading(path, frame, last if last is not None else 0.0)
            if kind == 'Blaze':
                angle = last if last is not None else 0.0
            last = angle
            arm.rotation_euler = (0.0, 0.0, angle - facing)
            arm.keyframe_insert('location', frame=frame + 1)
            arm.keyframe_insert('rotation_euler', frame=frame + 1)

    cam_data = bpy.data.cameras.new(scene.name + ' Camera')
    cam_data.sensor_fit = 'VERTICAL'
    cam_data.angle_y = math.radians(70.0)
    cam_data.clip_start = 0.05
    cam_data.clip_end = 1000.0
    cam = bpy.data.objects.new(scene.name + ' Camera', cam_data)
    scene.collection.objects.link(cam)
    cam.rotation_mode = 'QUATERNION'
    for frame, (eye, look) in sorted(cameras.items()):
        cam.location = eye
        cam.rotation_quaternion = (look - eye).to_track_quat('-Z', 'Y')
        cam.keyframe_insert('location', frame=frame + 1)
        cam.keyframe_insert('rotation_quaternion', frame=frame + 1)
    scene.camera = cam

    sun = bpy.data.lights.new(scene.name + ' Sun', 'SUN')
    sun.energy = 3.0 if index == 0 else 1.5
    sun_obj = bpy.data.objects.new(scene.name + ' Sun', sun)
    sun_obj.rotation_euler = (math.radians(40.0), math.radians(15.0), math.radians(30.0))
    scene.collection.objects.link(sun_obj)
    if 'portal' in info and index < 2:
        p = info['portal']
        marker = bpy.data.objects.new('Portal frame corner', None)
        marker.location = (p[0], -p[2], p[1])
        scene.collection.objects.link(marker)
    return scene, frames


def shot_file(index):
    return 'shot%d_%s.blend' % (index + 1, SHOT_NAMES[index].lower().replace(' ', '_'))


def build_scene_files(export_dir, assets, out_dir):
    made = []
    for index in range(4):
        folder = os.path.join(export_dir, 'shot%d' % index)
        if not os.path.isdir(folder):
            continue
        bpy.ops.wm.read_factory_settings(use_empty=True)
        atlas = bpy.data.images.load(os.path.join(export_dir, 'atlas.png'), check_existing=True)
        atlas.name = 'atlas.png'
        scene, frames = build_shot(index, folder, atlas, export_dir, assets)
        bpy.ops.file.pack_all()
        path = os.path.join(out_dir, shot_file(index))
        bpy.ops.wm.save_as_mainfile(filepath=path, compress=True)
        made.append((path, scene.name, frames))
    bpy.ops.wm.read_factory_settings(use_empty=True)
    edit = bpy.context.scene
    edit.name = 'Cinematic Edit'
    edit.render.fps = FPS
    edit.render.resolution_x = 1920
    edit.render.resolution_y = 1080
    edit.sequence_editor_create()
    strips = edit.sequence_editor.strips if hasattr(edit.sequence_editor, 'strips') else edit.sequence_editor.sequences
    at = 1
    for channel, (path, name, frames) in enumerate(made):
        with bpy.data.libraries.load(path, link=True, relative=True) as (src, dst):
            dst.scenes = [name]
        linked = dst.scenes[0]
        strips.new_scene(name, linked, 1 + (channel % 2), at)
        at += frames
    edit.frame_start = 1
    edit.frame_end = at - 1
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out_dir, 'cinematic_edit.blend'), relative_remap=True)


def build_rig_file(kind, assets, out_path):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.name = kind
    scene.render.fps = FPS
    collection = bpy.data.collections.new(kind)
    scene.collection.children.link(collection)
    if kind == 'Steve':
        rig = rigs.build_steve(collection, assets)
        rigs.walk_action(rig)
    elif kind == 'Ender Dragon':
        rig = rigs.build_dragon(collection, assets, 'Ender Dragon')
        rigs.animate_dragon(rig, 1, 60)
        scene.frame_end = 60
    else:
        rig = rigs.build_blaze(collection, assets, 'Blaze')
        rigs.animate_blaze(rig, 1, 60)
        scene.frame_end = 60
    bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=out_path, compress=True)


if __name__ == '__main__':
    args = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else sys.argv[1:]
    mode = args[0]
    if mode == 'scene':
        build_scene_files(args[1], args[2], args[3])
    else:
        build_rig_file(args[1], args[2], args[3])
