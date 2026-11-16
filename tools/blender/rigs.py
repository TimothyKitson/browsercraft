import math
import bpy
from mathutils import Matrix, Vector, Quaternion

CONVERT = Matrix(((1.0, 0.0, 0.0), (0.0, 0.0, 1.0), (0.0, -1.0, 0.0)))


def to_blender(p, scale=1.0 / 16.0, offset=(0.0, 0.0, 0.0)):
    return Vector((p[0] * scale + offset[0], p[2] * scale + offset[1], -p[1] * scale + offset[2]))


def model_rotation(pitch, yaw, roll):
    rz = Matrix.Rotation(roll, 3, 'Z')
    ry = Matrix.Rotation(yaw, 3, 'Y')
    rx = Matrix.Rotation(pitch, 3, 'X')
    return CONVERT @ (rz @ ry @ rx) @ CONVERT.transposed()


def cuboid_quads(origin, size, uv, tex, inflate=0.0, mirror=False):
    x, y, z = origin
    dx, dy, dz = size
    f = x + dx
    g = y + dy
    h = z + dz
    x -= inflate
    y -= inflate
    z -= inflate
    f += inflate
    g += inflate
    h += inflate
    if mirror:
        x, f = f, x
    v = [(x, y, z), (f, y, z), (f, g, z), (x, g, z), (x, y, h), (f, y, h), (f, g, h), (x, g, h)]
    u0, v0 = uv
    j = u0
    k = u0 + dz
    l = u0 + dz + dx
    m = u0 + dz + dx + dx
    n = u0 + dz + dx + dz
    o = u0 + dz + dx + dz + dx
    p = v0
    q = v0 + dz
    r = v0 + dz + dy
    faces = [((5, 4, 0, 1), k, p, l, q), ((2, 3, 7, 6), l, q, m, p), ((0, 4, 7, 3), j, q, k, r),
             ((1, 0, 3, 2), k, q, l, r), ((5, 1, 2, 6), l, q, n, r), ((4, 5, 6, 7), n, q, o, r)]
    out = []
    for idx, u1, v1, u2, v2 in faces:
        corners = [v[i] for i in idx]
        uvs = [(u2, v1), (u1, v1), (u1, v2), (u2, v2)]
        if mirror:
            corners = corners[::-1]
            uvs = uvs[::-1]
        out.append((corners, [(a / tex[0], 1.0 - b / tex[1]) for a, b in uvs]))
    return out


def image(path, name=None):
    img = bpy.data.images.load(path, check_existing=True)
    if name:
        img.name = name
    return img


def skin_material(name, img, glow=None):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    nt.nodes.clear()
    out = nt.nodes.new('ShaderNodeOutputMaterial')
    tex = nt.nodes.new('ShaderNodeTexImage')
    tex.image = img
    tex.interpolation = 'Closest'
    bsdf = nt.nodes.new('ShaderNodeBsdfPrincipled')
    bsdf.inputs['Roughness'].default_value = 0.9
    nt.links.new(tex.outputs['Color'], bsdf.inputs['Base Color'])
    nt.links.new(tex.outputs['Alpha'], bsdf.inputs['Alpha'])
    if glow is not None:
        eyes = nt.nodes.new('ShaderNodeTexImage')
        eyes.image = glow
        eyes.interpolation = 'Closest'
        mix = nt.nodes.new('ShaderNodeVectorMath')
        mix.operation = 'SCALE'
        nt.links.new(eyes.outputs['Color'], mix.inputs[0])
        nt.links.new(eyes.outputs['Alpha'], mix.inputs['Scale'])
        nt.links.new(mix.outputs['Vector'], bsdf.inputs['Emission Color'])
        bsdf.inputs['Emission Strength'].default_value = 4.0
    nt.links.new(bsdf.outputs['BSDF'], out.inputs['Surface'])
    if hasattr(mat, 'surface_render_method'):
        mat.surface_render_method = 'DITHERED'
    mat.use_backface_culling = False
    return mat


def mesh_object(name, quads, pivot, offset, mat, collection):
    verts = []
    faces = []
    uvs = []
    head = to_blender(pivot, offset=offset)
    for corners, corner_uv in quads:
        start = len(verts)
        for c in corners:
            verts.append(to_blender(c, offset=offset) - head)
        faces.append(tuple(range(start, start + 4)))
        uvs.extend(corner_uv)
    me = bpy.data.meshes.new(name)
    me.from_pydata([tuple(v) for v in verts], [], faces)
    layer = me.uv_layers.new(name='UVMap')
    for i, loop in enumerate(me.loops):
        layer.data[i].uv = uvs[loop.vertex_index]
    me.materials.append(mat)
    me.update()
    obj = bpy.data.objects.new(name, me)
    collection.objects.link(obj)
    obj.location = head
    return obj


class Rig:
    def __init__(self, name, collection, offset):
        self.name = name
        self.collection = collection
        self.offset = offset
        arm = bpy.data.armatures.new(name + 'Armature')
        arm.display_type = 'STICK'
        self.armature = bpy.data.objects.new(name, arm)
        collection.objects.link(self.armature)
        self.bones = {}
        self.parts = []

    def bone(self, name, pivot, parent=None):
        self.bones[name] = (to_blender(pivot, offset=self.offset), parent)

    def part(self, bone, name, cubes, pivot, mat):
        quads = []
        for cube in cubes:
            quads.extend(cuboid_quads(*cube))
        self.parts.append((bone, mesh_object(self.name + '_' + name, quads, pivot, self.offset, mat, self.collection)))

    def build(self):
        view = bpy.context.view_layer
        if self.armature.name not in view.objects:
            bpy.context.scene.collection.objects.link(self.armature)
        view.objects.active = self.armature
        bpy.ops.object.mode_set(mode='EDIT')
        edit = self.armature.data.edit_bones
        for name, (head, parent) in self.bones.items():
            b = edit.new(name)
            b.head = head
            b.tail = head + Vector((0.0, 0.0, 0.25))
            b.roll = 0.0
        for name, (head, parent) in self.bones.items():
            if parent:
                edit[name].parent = edit[parent]
        bpy.ops.object.mode_set(mode='OBJECT')
        if self.armature.name in bpy.context.scene.collection.objects:
            if self.armature.name in self.collection.objects:
                bpy.context.scene.collection.objects.unlink(self.armature)
        for bone, obj in self.parts:
            world = obj.matrix_world.copy()
            obj.parent = self.armature
            obj.parent_type = 'BONE'
            obj.parent_bone = bone
            obj.matrix_world = world
        for pb in self.armature.pose.bones:
            pb.rotation_mode = 'QUATERNION'
        return self.armature

    def pose(self, bone, pitch=0.0, yaw=0.0, roll=0.0, frame=None):
        pb = self.armature.pose.bones[bone]
        rest = pb.bone.matrix_local.to_3x3()
        r = rest.transposed() @ model_rotation(pitch, yaw, roll) @ rest
        pb.rotation_quaternion = r.to_quaternion()
        if frame is not None:
            pb.keyframe_insert('rotation_quaternion', frame=frame)


def build_steve(collection, assets, with_gear=True):
    skin = image(assets + '/textures/entity/player/wide/steve.png', 'steve.png')
    body = skin_material('Steve skin', skin)
    rig = Rig('Steve', collection, (0.0, 0.0, 1.5))
    rig.bone('root', (0, 24, 0))
    rig.bone('body', (0, 0, 0), 'root')
    rig.bone('head', (0, 0, 0), 'body')
    rig.bone('arm.R', (-5, 2, 0), 'body')
    rig.bone('arm.L', (5, 2, 0), 'body')
    rig.bone('leg.R', (-1.9, 12, 0), 'root')
    rig.bone('leg.L', (1.9, 12, 0), 'root')
    t = (64, 64)
    rig.part('head', 'head', [((-4, -8, -4), (8, 8, 8), (0, 0), t), ((-4, -8, -4), (8, 8, 8), (32, 0), t, 0.5)], (0, 0, 0), body)
    rig.part('body', 'body', [((-4, 0, -2), (8, 12, 4), (16, 16), t), ((-4, 0, -2), (8, 12, 4), (16, 32), t, 0.25)], (0, 0, 0), body)
    rig.part('arm.R', 'arm_right', [((-8, 0, -2), (4, 12, 4), (40, 16), t), ((-8, 0, -2), (4, 12, 4), (40, 32), t, 0.25)], (-5, 2, 0), body)
    rig.part('arm.L', 'arm_left', [((4, 0, -2), (4, 12, 4), (32, 48), t), ((4, 0, -2), (4, 12, 4), (48, 48), t, 0.25)], (5, 2, 0), body)
    rig.part('leg.R', 'leg_right', [((-3.9, 12, -2), (4, 12, 4), (0, 16), t), ((-3.9, 12, -2), (4, 12, 4), (0, 32), t, 0.25)], (-1.9, 12, 0), body)
    rig.part('leg.L', 'leg_left', [((-0.1, 12, -2), (4, 12, 4), (16, 48), t), ((-0.1, 12, -2), (4, 12, 4), (0, 48), t, 0.25)], (1.9, 12, 0), body)
    if with_gear:
        outer = skin_material('Diamond armor', image(assets + '/textures/models/armor/diamond_layer_1.png', 'diamond_layer_1.png'))
        inner = skin_material('Diamond leggings', image(assets + '/textures/models/armor/diamond_layer_2.png', 'diamond_layer_2.png'))
        a = (64, 32)
        rig.part('head', 'helmet', [((-4, -8, -4), (8, 8, 8), (0, 0), a, 1.0)], (0, 0, 0), outer)
        rig.part('body', 'chestplate', [((-4, 0, -2), (8, 12, 4), (16, 16), a, 1.0)], (0, 0, 0), outer)
        rig.part('arm.R', 'chestplate_arm_right', [((-8, 0, -2), (4, 12, 4), (40, 16), a, 1.0)], (-5, 2, 0), outer)
        rig.part('arm.L', 'chestplate_arm_left', [((4, 0, -2), (4, 12, 4), (40, 16), a, 1.0, True)], (5, 2, 0), outer)
        rig.part('body', 'leggings_waist', [((-4, 0, -2), (8, 12, 4), (16, 16), a, 0.5)], (0, 0, 0), inner)
        rig.part('leg.R', 'leggings_right', [((-3.9, 12, -2), (4, 12, 4), (0, 16), a, 0.4)], (-1.9, 12, 0), inner)
        rig.part('leg.L', 'leggings_left', [((-0.1, 12, -2), (4, 12, 4), (0, 16), a, 0.4, True)], (1.9, 12, 0), inner)
        rig.part('leg.R', 'boots_right', [((-3.9, 12, -2), (4, 12, 4), (0, 16), a, 0.9)], (-1.9, 12, 0), outer)
        rig.part('leg.L', 'boots_left', [((-0.1, 12, -2), (4, 12, 4), (0, 16), a, 0.9, True)], (1.9, 12, 0), outer)
    arm = rig.build()
    if with_gear:
        sword = build_sword(collection, assets)
        hand = Matrix.Translation((-5.0, 2.0, 0.0))
        hand = hand @ Matrix.Rotation(math.radians(-90.0), 4, 'X') @ Matrix.Rotation(math.radians(180.0), 4, 'Y')
        hand = hand @ Matrix.Translation((1.0, 2.0, -10.0))
        hand = hand @ Matrix.Translation((0.0, 4.0, 0.5))
        hand = hand @ Matrix.Rotation(math.radians(0.0), 4, 'X') @ Matrix.Rotation(math.radians(-90.0), 4, 'Y') @ Matrix.Rotation(math.radians(55.0), 4, 'Z')
        hand = hand @ Matrix.Scale(0.85 * 16.0, 4) @ Matrix.Translation((-0.5, -0.5, -0.5))
        convert = CONVERT.to_4x4()
        root = Matrix.Translation((0.0, 0.0, 1.5)) @ Matrix.Scale(1.0 / 16.0, 4) @ convert
        sword.matrix_world = root @ hand
        world = sword.matrix_world.copy()
        sword.parent = arm
        sword.parent_type = 'BONE'
        sword.parent_bone = 'arm.R'
        sword.matrix_world = world
    return rig


def build_sword(collection, assets):
    img = image(assets + '/textures/item/diamond_sword.png', 'diamond_sword.png')
    w, h = img.size
    px = list(img.pixels)
    solid = [[px[((h - 1 - y) * w + x) * 4 + 3] > 0.1 for x in range(w)] for y in range(h)]
    verts = []
    faces = []
    uvs = []
    s = 1.0 / 16.0
    z0, z1 = 7.5 * s, 8.5 * s

    def quad(corners, uv):
        start = len(verts)
        verts.extend(corners)
        faces.append((start, start + 1, start + 2, start + 3))
        uvs.extend(uv)

    for y in range(h):
        for x in range(w):
            if not solid[y][x]:
                continue
            x0, x1 = x * s * 16 / w, (x + 1) * s * 16 / w
            y0, y1 = 1.0 - (y + 1) * s * 16 / h, 1.0 - y * s * 16 / h
            cu = (x + 0.5) / w
            cv = 1.0 - (y + 0.5) / h
            pix = [(cu, cv)] * 4
            quad([(x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)], [((x) / w, 1 - (y + 1) / h), ((x + 1) / w, 1 - (y + 1) / h), ((x + 1) / w, 1 - y / h), (x / w, 1 - y / h)])
            quad([(x1, y0, z0), (x0, y0, z0), (x0, y1, z0), (x1, y1, z0)], [((x + 1) / w, 1 - (y + 1) / h), (x / w, 1 - (y + 1) / h), (x / w, 1 - y / h), ((x + 1) / w, 1 - y / h)])
            if x == 0 or not solid[y][x - 1]:
                quad([(x0, y0, z0), (x0, y0, z1), (x0, y1, z1), (x0, y1, z0)], pix)
            if x == w - 1 or not solid[y][x + 1]:
                quad([(x1, y0, z1), (x1, y0, z0), (x1, y1, z0), (x1, y1, z1)], pix)
            if y == 0 or not solid[y - 1][x]:
                quad([(x0, y1, z1), (x1, y1, z1), (x1, y1, z0), (x0, y1, z0)], pix)
            if y == h - 1 or not solid[y + 1][x]:
                quad([(x0, y0, z0), (x1, y0, z0), (x1, y0, z1), (x0, y0, z1)], pix)
    me = bpy.data.meshes.new('DiamondSword')
    me.from_pydata(verts, [], faces)
    layer = me.uv_layers.new(name='UVMap')
    for i, loop in enumerate(me.loops):
        layer.data[i].uv = uvs[loop.vertex_index]
    me.materials.append(skin_material('Diamond sword', img))
    me.update()
    obj = bpy.data.objects.new('DiamondSword', me)
    collection.objects.link(obj)
    return obj


def walk_action(rig, frames=24):
    arm = rig.armature
    for f in range(0, frames + 1, 3):
        phase = f / frames * 2.0 * math.pi
        swing = math.cos(phase) * 0.9
        rig.pose('leg.R', pitch=swing, frame=f + 1)
        rig.pose('leg.L', pitch=-swing, frame=f + 1)
        rig.pose('arm.R', pitch=-swing * 0.8, frame=f + 1)
        rig.pose('arm.L', pitch=swing * 0.8, frame=f + 1)
    action = arm.animation_data.action
    action.name = 'Steve_Walk'
    action.use_fake_user = True
    arm.animation_data.action = None
    for pb in arm.pose.bones:
        pb.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
    return action


def build_blaze(collection, assets, name='Blaze'):
    import os
    path = assets + '/skins/mob/blaze.png'
    if not os.path.exists(path):
        path = assets + '/textures/entity/blaze.png'
    mat = skin_material('Blaze skin', image(path, 'blaze.png'))
    rig = Rig(name, collection, (0.0, 0.0, 1.5))
    t = (64, 32)
    rig.bone('root', (0, 24, 0))
    rig.bone('head', (0, 0, 0), 'root')
    rig.part('head', 'head', [((-4, -4, -4), (8, 8, 8), (0, 0), t)], (0, 0, 0), mat)
    rest = []
    for i in range(12):
        rig.bone('rod%d' % i, (0, 0, 0), 'root')
        rig.part('rod%d' % i, 'rod%d' % i, [((0, 0, 0), (2, 8, 2), (0, 16), t)], (0, 0, 0), mat)
        rest.append(i)
    rig.build()
    return rig


def animate_blaze(rig, first, last, start_ticks=0.0, fps=30.0):
    arm = rig.armature
    for frame in range(first, last + 1):
        ap = start_ticks + (frame - first) * 20.0 / fps
        f = ap * math.pi * -0.1
        spots = []
        for i in range(4):
            spots.append((math.cos(f) * 9.0, -2.0 + math.cos((i * 2 + ap) * 0.25), math.sin(f) * 9.0))
            f += 1
        f = math.pi / 4 + ap * math.pi * 0.03
        for i in range(4, 8):
            spots.append((math.cos(f) * 7.0, 2.0 + math.cos((i * 2 + ap) * 0.25), math.sin(f) * 7.0))
            f += 1
        f = 0.47123894 + ap * math.pi * -0.05
        for i in range(8, 12):
            spots.append((math.cos(f) * 5.0, 11.0 + math.cos((i * 1.5 + ap) * 0.5), math.sin(f) * 5.0))
            f += 1
        for i, spot in enumerate(spots):
            pb = arm.pose.bones['rod%d' % i]
            pb.location = pb.bone.matrix_local.to_3x3().transposed() @ (to_blender(spot) - to_blender((0, 0, 0)))
            pb.keyframe_insert('location', frame=frame)
    arm.animation_data.action.name = rig.name + '_Rods'
    return arm.animation_data.action


NECK = [((-5, -5, -5), (10, 10, 10), (192, 104), (256, 256)), ((-1, -9, -3), (2, 4, 6), (48, 0), (256, 256))]


def build_dragon(collection, assets, name='EnderDragon'):
    img = image(assets + '/textures/entity/enderdragon/dragon.png', 'dragon.png')
    eyes = image(assets + '/textures/entity/enderdragon/dragon_eyes.png', 'dragon_eyes.png')
    mat = skin_material('Ender dragon', img, eyes)
    rig = Rig(name, collection, (0.0, -2.0, 3.5))
    t = (256, 256)
    rig.bone('root', (0, 24, 0))
    rig.bone('body', (0, 4, 8), 'root')
    rig.part('body', 'body', [((-12, 0, -16), (24, 24, 64), (0, 0), t), ((-1, -6, -10), (2, 6, 12), (220, 53), t),
                             ((-1, -6, 10), (2, 6, 12), (220, 53), t), ((-1, -6, 30), (2, 6, 12), (220, 53), t)], (0, 4, 8), mat)
    parent = 'root'
    for o in range(5):
        pivot = (0, 20, -12 - 10 * o)
        rig.bone('neck%d' % o, pivot, parent)
        rig.part('neck%d' % o, 'neck%d' % o, [(tuple(c[0][k] + pivot[k] for k in range(3)),) + c[1:] for c in NECK], pivot, mat)
        parent = 'neck%d' % o
    head = (0, 20, -62)
    rig.bone('head', head, parent)
    hc = [((-6, -1, -24), (12, 5, 16), (176, 44), t), ((-8, -8, -10), (16, 16, 16), (112, 30), t),
          ((-5, -12, -4), (2, 4, 6), (0, 0), t, 0.0, True), ((-5, -3, -22), (2, 2, 4), (112, 0), t, 0.0, True),
          ((3, -12, -4), (2, 4, 6), (0, 0), t, 0.0, True), ((3, -3, -22), (2, 2, 4), (112, 0), t, 0.0, True)]
    rig.part('head', 'head', [(tuple(c[0][k] + head[k] for k in range(3)),) + c[1:] for c in hc], head, mat)
    jaw = (0, 24, -70)
    rig.bone('jaw', jaw, 'head')
    rig.part('jaw', 'jaw', [((-6, 24, -86), (12, 4, 16), (176, 65), t)], jaw, mat)
    parent = 'body'
    for r in range(12):
        pivot = (0, 10, 60 + 10 * r)
        rig.bone('tail%d' % r, pivot, parent)
        cubes = []
        for c in NECK:
            lo = c[0]
            size = c[1]
            turned = (-lo[0] - size[0] + pivot[0], lo[1] + pivot[1], -lo[2] - size[2] + pivot[2])
            cubes.append((turned,) + c[1:])
        rig.part('tail%d' % r, 'tail%d' % r, cubes, pivot, mat)
        parent = 'tail%d' % r
    for side, sx, mirror in (('L', 1, True), ('R', -1, False)):
        wing = (12 * sx, 5, 2)
        tip = (68 * sx, 5, 2)
        rig.bone('wing.' + side, wing, 'body')
        rig.bone('wingtip.' + side, tip, 'wing.' + side)
        lo = 0 if sx > 0 else -56
        rig.part('wing.' + side, 'wing_' + side, [((wing[0] + lo, 1, -2), (56, 8, 8), (112, 88), t, 0.0, mirror),
                                                  ((wing[0] + lo, 5, 4), (56, 0, 56), (-56, 88), t, 0.0, mirror)], wing, mat)
        rig.part('wingtip.' + side, 'wingtip_' + side, [((tip[0] + lo, 3, 0), (56, 4, 4), (112, 136), t, 0.0, mirror),
                                                        ((tip[0] + lo, 5, 4), (56, 0, 56), (-56, 144), t, 0.0, mirror)], tip, mat)
        front = (12 * sx, 20, 2)
        rig.bone('leg_front.' + side, front, 'body')
        rig.part('leg_front.' + side, 'leg_front_' + side, [((front[0] - 4, 16, -2), (8, 24, 8), (112, 104), t)], front, mat)
        ftip = (front[0], 40, 1)
        rig.bone('leg_front_tip.' + side, ftip, 'leg_front.' + side)
        rig.part('leg_front_tip.' + side, 'leg_front_tip_' + side, [((ftip[0] - 3, 39, -2), (6, 24, 6), (226, 138), t)], ftip, mat)
        ffoot = (front[0], 63, 1)
        rig.bone('foot_front.' + side, ffoot, 'leg_front_tip.' + side)
        rig.part('foot_front.' + side, 'foot_front_' + side, [((ffoot[0] - 4, 63, -11), (8, 4, 16), (144, 104), t)], ffoot, mat)
        hind = (16 * sx, 16, 42)
        rig.bone('leg_hind.' + side, hind, 'body')
        rig.part('leg_hind.' + side, 'leg_hind_' + side, [((hind[0] - 8, 12, 34), (16, 32, 16), (0, 0), t)], hind, mat)
        htip = (hind[0], 48, 38)
        rig.bone('leg_hind_tip.' + side, htip, 'leg_hind.' + side)
        rig.part('leg_hind_tip.' + side, 'leg_hind_tip_' + side, [((htip[0] - 6, 46, 38), (12, 32, 12), (196, 0), t)], htip, mat)
        hfoot = (hind[0], 79, 42)
        rig.bone('foot_hind.' + side, hfoot, 'leg_hind_tip.' + side)
        rig.part('foot_hind.' + side, 'foot_hind_' + side, [((hfoot[0] - 9, 79, 22), (18, 6, 24), (112, 0), t)], hfoot, mat)
    rig.build()
    return rig


def animate_dragon(rig, first, last, start=0.0, rate=0.05, fps=30.0):
    arm = rig.armature
    for frame in range(first, last + 1, 2):
        ticks = (frame - first) * 20.0 / fps
        f = start + ticks * rate
        q = f * math.pi * 2.0
        g = math.sin(q - 1.0) + 1.0
        g = (g * g + g * 2.0) * 0.05
        rig.pose('jaw', pitch=(math.sin(q) + 1.0) * 0.2, frame=frame)
        pitch = 0.125 - math.cos(q) * 0.2
        roll = -(math.sin(q) + 0.125) * 0.8
        rig.pose('wing.L', pitch=pitch, yaw=-0.25, roll=roll, frame=frame)
        rig.pose('wing.R', pitch=pitch, yaw=0.25, roll=-roll, frame=frame)
        tip = (math.sin(q + 2.0) + 0.5) * 0.75
        rig.pose('wingtip.L', roll=tip, frame=frame)
        rig.pose('wingtip.R', roll=-tip, frame=frame)
        for o in range(5):
            rig.pose('neck%d' % o, pitch=math.cos(o * 0.45 + q) * 0.15 * 0.3, frame=frame)
        for r in range(12):
            rig.pose('tail%d' % r, pitch=math.sin(r * 0.45 + q) * 0.05 * 0.6, frame=frame)
        for side in ('L', 'R'):
            rig.pose('leg_hind.' + side, pitch=1.0 + g * 0.1, frame=frame)
            rig.pose('leg_hind_tip.' + side, pitch=0.5 + g * 0.1, frame=frame)
            rig.pose('foot_hind.' + side, pitch=0.75 + g * 0.1, frame=frame)
            rig.pose('leg_front.' + side, pitch=1.3 + g * 0.1, frame=frame)
            rig.pose('leg_front_tip.' + side, pitch=-0.5 - g * 0.1, frame=frame)
            rig.pose('foot_front.' + side, pitch=0.75 + g * 0.1, frame=frame)
    arm.animation_data.action.name = rig.name + '_Flight'
