import hashlib
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

def spelled(word, vocabulary):
    reach = [False] * (len(word) + 1)
    reach[0] = True
    for start in range(len(word)):
        if not reach[start]:
            continue
        digits = re.match(r"\d+", word[start:])
        if digits:
            reach[start + len(digits.group(0))] = True
        for end in range(start + 1, len(word) + 1):
            if word[start:end] in vocabulary:
                reach[end] = True
    return reach[len(word)]

def tidy(literal):
    literal = literal.split("#", 1)[0]
    if literal.startswith("assets/textures/"):
        literal = literal[len("assets/textures/"):]
    while literal.startswith("../"):
        literal = literal[3:]
    return literal

def nearby(source, position):
    lines = source.count("\n", 0, position)
    split = source.split("\n")
    window = "\n".join(split[max(0, lines - 60):lines + 6])
    words = set()
    for literal in re.findall(r'"([^"\n]*)"', window):
        for part in literal.replace("|", "/").split("/"):
            part = part.replace(".png", "")
            if re.fullmatch(r"[a-z0-9_]+", part):
                words.add(part)
    return words

patterns = []
def remember(prefix, suffix, words):
    prefix, suffix = tidy(prefix), tidy(suffix).replace(".png", "")
    if len(prefix) < 3 or not re.search(r"[a-z]", prefix) or not re.fullmatch(r"[a-z0-9_/]+", prefix):
        return
    if suffix and not re.fullmatch(r"[a-z0-9_/]+", suffix):
        return
    patterns.append((re.compile(r"(?:^|/)" + re.escape(prefix) + r"([a-z0-9_]+)" + re.escape(suffix) + r"$"), words))

for source in sources:
    for found in re.finditer(r'"([^"\n]*)"\s*\+\s*[^;"\n]+?\s*\+\s*"([^"\n]*)"', source):
        remember(found.group(1), found.group(2), nearby(source, found.start()))
    for found in re.finditer(r'"([^"\n]*)"\s*\+\s*(?:std::to_string|[A-Za-z_(])', source):
        remember(found.group(1), "", nearby(source, found.start()))
    for found in re.finditer(r'"([^"\n]*%[dsu][^"\n]*)"', source):
        head, _, tail = re.split(r"(%[dsu])", found.group(1), maxsplit=1)
        remember(head, tail, nearby(source, found.start()))

def built_in_code(path):
    for pattern, words in patterns:
        found = pattern.search(path)
        if found and spelled(found.group(1), words):
            return True
    return False

installed = set()
skins = os.path.join(root, "assets", "skins")
for base, _, files in os.walk(skins):
    for name in files:
        if name.endswith(".png"):
            with open(os.path.join(base, name), "rb") as handle:
                installed.add(hashlib.sha1(handle.read()).hexdigest())

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
        with open(os.path.join(pack, rel), "rb") as handle:
            if hashlib.sha1(handle.read()).hexdigest() in installed:
                return True
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
            if not used(category, rel) and not built_in_code(rel[:-4]):
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
