import os
import re
import sys

root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
pack = sys.argv[1] if len(sys.argv) > 1 else os.path.join(root, "assets", "textures")
show = sys.argv[2] if len(sys.argv) > 2 else ""

sources = []
for folder in ("src", "tools"):
    for base, _, files in os.walk(os.path.join(root, folder)):
        for name in files:
            if name.endswith((".cpp", ".h", ".inl", ".sh", ".ps1")):
                with open(os.path.join(base, name), encoding="utf-8", errors="replace") as handle:
                    sources.append(handle.read())
text = "\n".join(sources)
quoted = set(re.findall(r'"([^"\n]{1,120})"', text))
quoted_stems = set()
for value in quoted:
    for part in value.replace("|", "/").split("/"):
        quoted_stems.add(part.replace(".png", ""))

def used(category, rel):
    stem = os.path.basename(rel)[:-4]
    path = rel[:-4]
    inner = path.split("/", 1)[1] if "/" in path else path
    if category in ("block", "item"):
        return stem in quoted or ("../" + path) in text or (path + ".png") in text
    if category == "gui":
        return ('"' + inner + '"') in text or ('"' + inner + "|") in text
    if category == "particle":
        base = re.sub(r"_\d+$", "", stem)
        if stem.startswith("sga_"):
            base = "sga_"
        return ('"' + base + '"') in text
    if category == "entity":
        return (inner + ".png") in text or ('"' + path + '"') in text or ("../" + path + '"') in text
    return ('"' + path + '"') in text or ('"../' + path + '"') in text or (path + ".png") in text

skip_dirs = {"block.", "item."}
report = {}
for category in sorted(os.listdir(pack)):
    folder = os.path.join(pack, category)
    if not os.path.isdir(folder) or any(category.startswith(s) for s in skip_dirs):
        continue
    for base, _, files in os.walk(folder):
        for name in files:
            if not name.endswith(".png") or name.endswith(("_n.png", "_s.png")):
                continue
            rel = os.path.relpath(os.path.join(base, name), pack).replace(os.sep, "/")
            report.setdefault(category, [0, []])
            report[category][0] += 1
            if not used(category, rel):
                report[category][1].append(rel)

total = sum(entry[0] for entry in report.values())
missing = sum(len(entry[1]) for entry in report.values())
print(f"{total} pack textures, {total - missing} attached, {missing} not attached")
for category, (count, unused) in sorted(report.items()):
    print(f"  {category:12s} {count - len(unused):5d} of {count:5d} attached")
if show:
    for category, (count, unused) in sorted(report.items()):
        if show != "all" and category != show:
            continue
        for rel in sorted(unused):
            print("   ", rel)
