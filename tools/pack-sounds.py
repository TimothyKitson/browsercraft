#!/usr/bin/env python3

import argparse
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
JS_PATH = os.path.join(ROOT, "index.js")
DATA_PATH = os.path.join(ROOT, "index.data")
SOUNDS_DIR = os.path.join(ROOT, "sounds")
SOUND_PREFIX = "/assets/sounds/"

ENTRY_RE = re.compile(r'filename:"([^"]+)",start:(\d+),end:(\d+)')
SIZE_RE = re.compile(r'remote_package_size:(\d+)')


def locate_manifest(js):
    anchor = js.find("loadPackage({files:[")
    if anchor < 0:
        sys.exit("error: could not find loadPackage({files:[ in index.js")
    start = js.index("[", anchor)
    depth = 0
    for i in range(start, len(js)):
        if js[i] == "[":
            depth += 1
        elif js[i] == "]":
            depth -= 1
            if depth == 0:
                return start, i + 1
    sys.exit("error: unterminated manifest array in index.js")


def parse_entries(block):
    entries = [(m.group(1), int(m.group(2)), int(m.group(3))) for m in ENTRY_RE.finditer(block)]
    if not entries:
        sys.exit("error: manifest contained no file entries")
    return entries


def replacement_path(filename):
    return os.path.join(SOUNDS_DIR, filename[len(SOUND_PREFIX):])


def main():
    ap = argparse.ArgumentParser(
        description="Repack sounds/ into index.data and patch the offsets in index.js.")
    ap.add_argument("--check", action="store_true",
                    help="report what would change, write nothing")
    args = ap.parse_args()

    js = open(JS_PATH, encoding="utf-8").read()
    data = open(DATA_PATH, "rb").read()

    m_start, m_end = locate_manifest(js)
    entries = parse_entries(js[m_start:m_end])

    expected = {f for f, _, _ in entries if f.startswith(SOUND_PREFIX)}
    on_disk = set()
    if os.path.isdir(SOUNDS_DIR):
        for dirpath, _, names in os.walk(SOUNDS_DIR):
            for name in names:
                if name.endswith(".ogg"):
                    rel = os.path.relpath(os.path.join(dirpath, name), SOUNDS_DIR)
                    on_disk.add(SOUND_PREFIX + rel.replace(os.sep, "/"))

    missing = sorted(expected - on_disk)
    extra = sorted(on_disk - expected)
    for f in missing:
        print("missing, keeping packed version: " + f[len(SOUND_PREFIX):])
    for f in extra:
        print("ignored, engine never loads it: " + f[len(SOUND_PREFIX):])

    blobs = []
    replaced = []
    cursor = 0
    for filename, start, end in entries:
        if filename.startswith(SOUND_PREFIX) and filename in on_disk:
            payload = open(replacement_path(filename), "rb").read()
            if payload[:4] != b"OggS":
                sys.exit("error: not an Ogg file: " + filename[len(SOUND_PREFIX):])
            if payload != data[start:end]:
                replaced.append(filename[len(SOUND_PREFIX):])
        else:
            payload = data[start:end]
        blobs.append((filename, cursor, cursor + len(payload), payload))
        cursor += len(payload)

    new_data = b"".join(b for _, _, _, b in blobs)
    new_manifest = "[" + ",".join(
        '{{filename:"{0}",start:{1},end:{2}}}'.format(f, s, e) for f, s, e, _ in blobs) + "]"
    new_js = js[:m_start] + new_manifest + js[m_end:]
    new_js, n = SIZE_RE.subn("remote_package_size:{0}".format(len(new_data)), new_js)
    if n != 1:
        sys.exit("error: expected exactly one remote_package_size, found {0}".format(n))

    print("")
    print("replaced {0} of {1} sounds".format(len(replaced), len(expected)))
    for f in replaced:
        print("  " + f)
    print("index.data: {0} -> {1} bytes".format(len(data), len(new_data)))

    if args.check:
        print("\n--check: nothing written")
        return
    if not replaced and len(new_data) == len(data):
        print("\nnothing to do")
        return

    open(DATA_PATH, "wb").write(new_data)
    open(JS_PATH, "w", encoding="utf-8").write(new_js)
    print("\nwrote index.data and index.js")


if __name__ == "__main__":
    main()
