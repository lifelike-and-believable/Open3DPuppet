#!/usr/bin/env python3
"""Build the Fab source-only zip of the Open3DPuppet plugin and check it.

Ported from Open3DBroadcast's script of the same name, without its WebRTC,
o3ds core and video codec checks, which do not apply here, and with the path
checks of rule FAB-003 added.

The package is made from the files git tracks under the plugin folder, so
nothing that is gitignored or generated (Binaries/, Intermediate/) can reach
it. Then:

1. Modules listed in Build/Fab/exclude-modules.txt are removed: their
   Source/<Module>/ folder is dropped and their entry is removed from the
   staged .uplugin. The .uplugin edit is textual, so the rest of the file
   keeps its formatting.
2. Files matching Build/Fab/exclude-files.txt are dropped.
3. The zip is written with a single top-level folder named after the plugin
   and fixed timestamps, so the same input gives the same bytes.
4. The zip is opened again and checked (see check_package). The tracked
   tree is also checked before any exclusion (see check_tree), so a
   committed .pdb, a Python script, or a developer note under Source/
   fails even where a rule would leave it out of the zip.
   Any problem fails the script with exit code 1.

Outputs in --out-dir:
  <zip-name>              the package
  <zip-name>.manifest.txt one line per file: sha256, size, path
  stage/<Plugin>/         the staged tree the zip was made from

Python 3.8 or later, standard library only. Exit codes: 0 package written
and checked; 1 a check failed; 2 bad input (missing file, not a git
checkout, unreadable .uplugin). --self-test runs the checks against made-up
file lists, clean and bad, and exits 0 only if each passes the clean one and
reports every problem in the bad one.
"""

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import zipfile

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DEFAULT_PLUGIN_DIR = os.path.join(REPO_ROOT, "ProjectSandbox", "Plugins", "Open3DPuppet")
DEFAULT_EXCLUDE_MODULES = os.path.join(REPO_ROOT, "Build", "Fab", "exclude-modules.txt")
DEFAULT_EXCLUDE_FILES = os.path.join(REPO_ROOT, "Build", "Fab", "exclude-files.txt")

# Fixed zip timestamp (the earliest the zip format can store).
ZIP_DATE_TIME = (1980, 1, 1, 0, 0, 0)

# Checked on the finished zip regardless of exclude-files.txt.
FORBIDDEN_FILE_GLOBS = ["**/*.pdb", "**/*.py", "**/*.pyc"]
# Checked on every git-tracked file under the plugin, before any exclusion, so
# neither exclude-files.txt nor an excluded module can hide one. Debug symbols
# and scripts are not kept in the plugin at all; executables and installers
# are never accepted by Fab (FAB-003).
FORBIDDEN_TREE_GLOBS = ["**/*.pdb", "**/*.py", "**/*.pyc", "**/*.exe", "**/*.msi"]
# Markdown allowed under Source/ outside a ThirdParty/ folder: a module's
# user-facing README.md or USER_GUIDE.md. Design notes live in docs/.
ALLOWED_SOURCE_MARKDOWN = {"README.md", "USER_GUIDE.md"}
FORBIDDEN_TOP_DIRS = ["Binaries", "Intermediate", "Saved", "DerivedDataCache"]
# Markdown allowed in the package: end-user docs at the plugin root and
# anything under a ThirdParty/ folder (licences and notices). The plugin has
# no root Markdown today; its documentation is on GitHub (DocsURL).
ALLOWED_ROOT_MARKDOWN = {"README.md", "USER_GUIDE.md", "THIRD_PARTY_LICENSES.md"}
REQUIRED_FILES = [
    "Resources/Icon128.png",
    "Config/FilterPlugin.ini",
    "LICENSE",
]
# Top-level folders BuildPlugin packages without a FilterPlugin.ini entry. Anything
# else at the top level must match a Config/FilterPlugin.ini rule, or BuildPlugin
# leaves it out of the package it compiles (FAB-003).
STANDARD_TOP_DIRS = {"Binaries", "Config", "Content", "Resources", "Shaders", "Source"}
FILTER_PLUGIN_INI = "Config/FilterPlugin.ini"
# Fab's limit on a path inside the plugin folder (FAB-003).
MAX_PATH_LENGTH = 170
# FAB-003: English letters, digits and underscores in folder and file names. A
# file name may have dot-separated parts (Open3DPuppet.Build.cs).
NAME_SEGMENT_RE = re.compile(r"^[A-Za-z0-9_]+$")


class InputError(Exception):
    pass


def glob_to_regex(pattern):
    """Translate a glob to a regex. '*' stays in one segment, '**' spans segments."""
    out = []
    i = 0
    while i < len(pattern):
        c = pattern[i]
        if pattern.startswith("**/", i):
            out.append("(?:.*/)?")
            i += 3
        elif pattern.startswith("**", i):
            out.append(".*")
            i += 2
        elif c == "*":
            out.append("[^/]*")
            i += 1
        elif c == "?":
            out.append("[^/]")
            i += 1
        else:
            out.append(re.escape(c))
            i += 1
    return re.compile("^" + "".join(out) + "$", re.IGNORECASE)


def read_list(path):
    if not os.path.isfile(path):
        raise InputError(f"list file not found: {path}")
    items = []
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.split("#", 1)[0].strip()
            if line:
                items.append(line)
    return items


def git_tracked_files(plugin_dir):
    """Paths (relative to plugin_dir, forward slashes) that git tracks."""
    try:
        top = subprocess.run(["git", "-C", plugin_dir, "rev-parse", "--show-toplevel"],
                             check=True, capture_output=True, text=True).stdout.strip()
        out = subprocess.run(["git", "-C", top, "ls-files", "-z", "--", os.path.relpath(plugin_dir, top)],
                             check=True, capture_output=True).stdout
    except (OSError, subprocess.CalledProcessError) as e:
        raise InputError(f"cannot list git-tracked files under {plugin_dir}: {e}")
    prefix = os.path.relpath(plugin_dir, top).replace(os.sep, "/").rstrip("/") + "/"
    files = []
    for raw in out.split(b"\0"):
        if not raw:
            continue
        p = raw.decode("utf-8")
        if not p.startswith(prefix):
            continue
        rel = p[len(prefix):]
        if os.path.isfile(os.path.join(top, p)):  # skip files deleted in the working tree
            files.append(rel)
    if not files:
        raise InputError(f"git tracks no files under {plugin_dir}")
    return sorted(files)


def _skip_ws(text, i):
    while i < len(text) and text[i] in " \t\r\n":
        i += 1
    return i


def _scan_value_end(text, i):
    """Index just past the JSON value starting at text[i] (object, array, string or scalar)."""
    if text[i] == '"':
        i += 1
        while text[i] != '"':
            i += 2 if text[i] == "\\" else 1
        return i + 1
    if text[i] in "{[":
        depth = 0
        in_str = False
        while True:
            c = text[i]
            if in_str:
                if c == "\\":
                    i += 1
                elif c == '"':
                    in_str = False
            elif c == '"':
                in_str = True
            elif c in "{[":
                depth += 1
            elif c in "}]":
                depth -= 1
                if depth == 0:
                    return i + 1
            i += 1
    while i < len(text) and text[i] not in ",}] \t\r\n":
        i += 1
    return i


def remove_modules_from_uplugin(text, names):
    """Remove Modules[] entries whose Name is in names, keeping all other text as is."""
    data = json.loads(text)
    wanted = [m for m in data.get("Modules", []) if m.get("Name") not in names]
    if len(wanted) == len(data.get("Modules", [])):
        return text

    # Find the top-level "Modules" array.
    i = _skip_ws(text, 0)
    if text[i] != "{":
        raise InputError(".uplugin does not start with an object")
    i += 1
    arr_open = None
    while True:
        i = _skip_ws(text, i)
        if text[i] == "}":
            break
        key_end = _scan_value_end(text, i)
        key = json.loads(text[i:key_end])
        i = _skip_ws(text, key_end)
        assert text[i] == ":"
        i = _skip_ws(text, i + 1)
        if key == "Modules":
            arr_open = i
            break
        i = _skip_ws(text, _scan_value_end(text, i))
        if text[i] == ",":
            i += 1
    if arr_open is None or text[arr_open] != "[":
        raise InputError(".uplugin has no Modules array")

    arr_close = _scan_value_end(text, arr_open) - 1
    spans = []
    i = _skip_ws(text, arr_open + 1)
    while i < arr_close:
        end = _scan_value_end(text, i)
        spans.append((i, end))
        i = _skip_ws(text, end)
        if text[i] == ",":
            i = _skip_ws(text, i + 1)

    keep = [s for s in spans if json.loads(text[s[0]:s[1]]).get("Name") not in names]
    sep = text[spans[0][1]:spans[1][0]] if len(spans) > 1 else ",\n"
    body = sep.join(text[s:e] for s, e in keep)
    lead = text[arr_open + 1:spans[0][0]] if spans else ""
    tail = text[spans[-1][1]:arr_close] if spans else ""
    if not keep:
        lead = ""
    result = text[:arr_open + 1] + lead + body + tail + text[arr_close:]

    check = json.loads(result)
    expected = dict(data)
    expected["Modules"] = wanted
    if check != expected:
        raise InputError("internal error: .uplugin edit changed more than the Modules entries")
    return result


def stage(plugin_dir, stage_root, excluded_modules, exclude_globs):
    plugin_name = None
    for f in os.listdir(plugin_dir):
        if f.endswith(".uplugin"):
            plugin_name = f[:-len(".uplugin")]
    if not plugin_name:
        raise InputError(f"no .uplugin in {plugin_dir}")

    files = git_tracked_files(plugin_dir)
    tree_errors = check_tree(files)
    present_modules = {p.split("/")[1] for p in files if p.startswith("Source/") and p.count("/") >= 2}
    report = {"removed_modules": [], "absent_modules": [], "excluded_files": [],
              "tree_errors": tree_errors}
    for m in excluded_modules:
        (report["removed_modules"] if m in present_modules else report["absent_modules"]).append(m)

    patterns = [glob_to_regex(g) for g in exclude_globs]
    kept = []
    for rel in files:
        parts = rel.split("/")
        if len(parts) > 2 and parts[0] == "Source" and parts[1] in excluded_modules:
            continue
        if any(p.match(rel) for p in patterns):
            report["excluded_files"].append(rel)
            continue
        kept.append(rel)

    dest = os.path.join(stage_root, plugin_name)
    if os.path.exists(dest):
        shutil.rmtree(dest)
    for rel in kept:
        src = os.path.join(plugin_dir, rel)
        dst = os.path.join(dest, rel)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copyfile(src, dst)

    uplugin_rel = plugin_name + ".uplugin"
    with open(os.path.join(plugin_dir, uplugin_rel), encoding="utf-8-sig", newline="") as f:
        text = f.read()
    try:
        edited = remove_modules_from_uplugin(text, set(excluded_modules))
    except json.JSONDecodeError as e:
        raise InputError(f"{uplugin_rel} is not valid JSON: {e}")
    with open(os.path.join(dest, uplugin_rel), "w", encoding="utf-8", newline="") as f:
        f.write(edited)
    return plugin_name, dest, sorted(kept), report


def write_zip(stage_dir, plugin_name, files, zip_path):
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for rel in files:
            info = zipfile.ZipInfo(f"{plugin_name}/{rel}", date_time=ZIP_DATE_TIME)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            with open(os.path.join(stage_dir, rel), "rb") as f:
                z.writestr(info, f.read())


def check_tree(files):
    """Return problems in the tracked plugin tree itself, before any exclusion.

    A file reported here fails the job even when an exclude-files.txt rule or
    an excluded module would keep it out of the zip: it must not be committed
    under the plugin at all.
    - FORBIDDEN_TREE_GLOBS: debug symbols, scripts, executables and installers.
    - Markdown under Source/, outside a ThirdParty/ folder, whose name is not
      in ALLOWED_SOURCE_MARKDOWN: developer notes.
    - Paths and names that break FAB-003 (check_fab_paths).
    """
    errors = []
    forbidden = [glob_to_regex(g) for g in FORBIDDEN_TREE_GLOBS]
    bad = [rel for rel in files if any(p.match(rel) for p in forbidden)]
    if bad:
        errors.append("debug symbols, scripts, executables or installers are tracked in the plugin tree; "
                      f"keep them out of the plugin (Build/README.md, \"Fab source package\"): {bad}")

    bad = [rel for rel in files
           if rel.startswith("Source/") and rel.lower().endswith(".md")
           and "/ThirdParty/" not in rel
           and rel.rsplit("/", 1)[-1] not in ALLOWED_SOURCE_MARKDOWN]
    if bad:
        errors.append("developer documents are tracked under Source/; move them to docs/ "
                      f"(only {', '.join(sorted(ALLOWED_SOURCE_MARKDOWN))} may live there, "
                      f"plus anything under ThirdParty/): {bad}")

    errors.extend(check_fab_paths(files))
    return errors


def check_fab_paths(files):
    """Paths over MAX_PATH_LENGTH, and names other than letters, digits and underscores (FAB-003)."""
    errors = []
    for rel in files:
        if len(rel) > MAX_PATH_LENGTH:
            errors.append(f"path is {len(rel)} characters, over Fab's {MAX_PATH_LENGTH} (FAB-003): {rel}")
        if not all(NAME_SEGMENT_RE.match(part) for name in rel.split("/") for part in name.split(".")):
            errors.append("name uses a character other than English letters, digits and underscores "
                          f"(FAB-003): {rel}")
    return errors


def filter_plugin_rules(text):
    """Regexes for the [FilterPlugin] entries of a FilterPlugin.ini ('...', '*' and '?' wildcards)."""
    rules = []
    in_section = False
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith(";"):
            continue
        if line.startswith("["):
            in_section = line.lower() == "[filterplugin]"
            continue
        if not in_section:
            continue
        out = []
        i = 0
        pattern = line.lstrip("/")
        while i < len(pattern):
            if pattern.startswith("...", i):
                out.append(".*")
                i += 3
            elif pattern[i] == "*":
                out.append("[^/]*")
                i += 1
            elif pattern[i] == "?":
                out.append("[^/]")
                i += 1
            else:
                out.append(re.escape(pattern[i]))
                i += 1
        rules.append(re.compile("^" + "".join(out) + "$", re.IGNORECASE))
    return rules


def check_top_level(rels, uplugin_rel, filter_text):
    """Top-level entries outside the standard folders must be listed in FilterPlugin.ini (FAB-003)."""
    rules = filter_plugin_rules(filter_text) if filter_text is not None else []
    unlisted = sorted(r for r in rels
                      if r != uplugin_rel and r.split("/")[0] not in STANDARD_TOP_DIRS
                      and not any(p.match(r) for p in rules))
    if unlisted:
        return [f"files outside {', '.join(sorted(STANDARD_TOP_DIRS))} that {FILTER_PLUGIN_INI} does not list, "
                f"so BuildPlugin would not package them (FAB-003): {unlisted[:10]}"]
    return []


def check_platforms(desc, uplugin_rel):
    """Platform declarations required by ADR 0001 (rule OP-005).

    The plugin has a non-empty SupportedTargetPlatforms, and every module has a
    non-empty PlatformAllowList that names only platforms the plugin supports,
    so a target for any other platform leaves the module out instead of
    building it.
    """
    errors = []
    supported = desc.get("SupportedTargetPlatforms")
    if not isinstance(supported, list) or not supported:
        errors.append(f"{uplugin_rel} has no SupportedTargetPlatforms (ADR 0001)")
        supported = []
    for m in desc.get("Modules", []):
        allow = m.get("PlatformAllowList")
        if not isinstance(allow, list) or not allow:
            errors.append(f"module {m.get('Name')} has no PlatformAllowList in {uplugin_rel} (ADR 0001)")
            continue
        extra = [p for p in allow if supported and p not in supported]
        if extra:
            errors.append(f"module {m.get('Name')} allows {extra}, which SupportedTargetPlatforms "
                          f"{supported} does not list (ADR 0001)")
    return errors


def check_package(zip_path, plugin_name, excluded_modules):
    """Return a list of problems found in the zip. Empty means it passes."""
    errors = []
    with zipfile.ZipFile(zip_path) as z:
        names = [n for n in z.namelist() if not n.endswith("/")]
        root = plugin_name + "/"
        outside = [n for n in names if not n.startswith(root)]
        if outside:
            errors.append(f"entries outside the {root} folder: {outside[:5]}")
        rels = [n[len(root):] for n in names if n.startswith(root)]
        relset = set(rels)

        uplugin_rel = plugin_name + ".uplugin"
        modules = []
        if uplugin_rel not in relset:
            errors.append(f"{uplugin_rel} is missing")
        else:
            try:
                desc = json.loads(z.read(root + uplugin_rel).decode("utf-8-sig"))
                modules = [m.get("Name") for m in desc.get("Modules", [])]
                errors.extend(check_platforms(desc, uplugin_rel))
            except (ValueError, UnicodeDecodeError) as e:
                errors.append(f"{uplugin_rel} is not valid JSON: {e}")
        if uplugin_rel in relset and not modules:
            errors.append(f"{uplugin_rel} lists no modules")
        filter_text = None
        if FILTER_PLUGIN_INI in relset:
            filter_text = z.read(root + FILTER_PLUGIN_INI).decode("utf-8-sig", errors="replace")
        errors.extend(check_top_level(rels, uplugin_rel, filter_text))

    for m in modules:
        if m in excluded_modules:
            errors.append(f"{uplugin_rel} still lists excluded module {m}")
        elif f"Source/{m}/{m}.Build.cs" not in relset:
            errors.append(f"module {m} is listed but Source/{m}/{m}.Build.cs is not in the package")
    for m in excluded_modules:
        leaked = [r for r in rels if r.startswith(f"Source/{m}/")]
        if leaked:
            errors.append(f"excluded module {m} has files in the package: {leaked[:5]}")

    top_dirs = {r.split("/")[0] for r in rels if "/" in r}
    for d in FORBIDDEN_TOP_DIRS:
        if d in top_dirs:
            errors.append(f"{d}/ must not be in a source package")

    forbidden = [glob_to_regex(g) for g in FORBIDDEN_FILE_GLOBS]
    bad = [r for r in rels if any(p.match(r) for p in forbidden)]
    if bad:
        errors.append(f"debug symbols or scripts in the package: {bad}")

    md = [r for r in rels if r.lower().endswith(".md")]
    dev_docs = [r for r in md
                if not (r in ALLOWED_ROOT_MARKDOWN or "/ThirdParty/" in "/" + r)]
    if dev_docs:
        errors.append("developer documents in the package (add an exclude-files.txt rule, "
                      f"move them out of the plugin, or allow them in fab-package.py): {dev_docs}")

    for req in REQUIRED_FILES:
        if req not in relset:
            errors.append(f"required file missing: {req}")
    return errors


def self_test():
    """The tree and descriptor checks against made-up inputs, clean and bad."""
    failures = []

    def expect(label, errors, count):
        print(f"  {label}: {len(errors)} problem(s)")
        for e in errors:
            print(f"    {e}")
        if len(errors) != count:
            failures.append(f"{label}: expected {count} problem(s), got {len(errors)}")

    clean = [
        "Open3DPuppet.uplugin",
        "LICENSE",
        "Config/FilterPlugin.ini",
        "Resources/Icon128.png",
        "Source/Open3DPuppet/Open3DPuppet.Build.cs",
        "Source/Open3DPuppet/Public/PuppetAsset.h",
        "Source/Open3DPuppet/README.md",
        "Source/ThirdParty/Lib/Notes.md",
    ]
    expect("clean tree", check_tree(clean), 0)
    expect("a script and an executable", check_tree(clean + ["Source/Tool.py", "Resources/Setup.exe"]), 1)
    expect("a design note under Source/", check_tree(clean + ["Source/Open3DPuppet/Design.md"]), 1)
    long_path = "Source/Open3DPuppet/Private/" + "A" * (MAX_PATH_LENGTH - len("Source/Open3DPuppet/Private/") - 1) + ".h"
    expect("a path over the limit", check_fab_paths([long_path]), 1)
    expect("a path at the limit", check_fab_paths([long_path[:-3] + ".h"]), 0)
    expect("a hyphen, a space, a non-English letter",
           check_fab_paths(["Source/My-Module/A.h", "Resources/Icon 128.png", "Source/X/Café.h"]), 3)
    expect("an empty name part", check_fab_paths(["Source/X/Bad..h"]), 1)

    desc = {"SupportedTargetPlatforms": ["Win64"], "Modules": [
        {"Name": "A", "PlatformAllowList": ["Win64"]},
        {"Name": "B"},
        {"Name": "C", "PlatformAllowList": ["Win64", "Linux"]},
    ]}
    expect("platforms", check_platforms(desc, "X.uplugin"), 2)
    expect("top level, LICENSE listed", check_top_level(clean + ["Docs/Guide.pdf"], "Open3DPuppet.uplugin", "[FilterPlugin]\n/LICENSE\n"), 1)

    text = '{\n\t"Modules": [\n\t\t{ "Name": "A" },\n\t\t{ "Name": "Tests" }\n\t],\n\t"X": 1\n}\n'
    edited = remove_modules_from_uplugin(text, {"Tests"})
    kept_format = json.loads(edited) == {"Modules": [{"Name": "A"}], "X": 1} and '\t\t{ "Name": "A" }' in edited
    print(f"  module removal keeps the formatting: {kept_format}")
    if not kept_format:
        failures.append(f"module removal: unexpected result {edited!r}")

    for f in failures:
        print(f"FAILED: {f}")
    print("self-test " + ("passed" if not failures else "FAILED"))
    return 0 if not failures else 1


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--plugin-dir", default=DEFAULT_PLUGIN_DIR)
    ap.add_argument("--exclude-modules", default=DEFAULT_EXCLUDE_MODULES)
    ap.add_argument("--exclude-files", default=DEFAULT_EXCLUDE_FILES)
    ap.add_argument("--out-dir")
    ap.add_argument("--zip-name", default=None, help="default: <Plugin>-Fab-Source.zip")
    ap.add_argument("--self-test", action="store_true", help="check the checks against made-up inputs")
    args = ap.parse_args(argv)

    if args.self_test:
        return self_test()
    if not args.out_dir:
        ap.error("--out-dir is required")

    try:
        excluded_modules = read_list(args.exclude_modules)
        exclude_globs = read_list(args.exclude_files)
        os.makedirs(args.out_dir, exist_ok=True)
        stage_root = os.path.join(args.out_dir, "stage")
        plugin_name, stage_dir, files, report = stage(
            os.path.abspath(args.plugin_dir), stage_root, excluded_modules, exclude_globs)
    except InputError as e:
        print(f"::error::fab-package: {e}")
        return 2

    zip_name = args.zip_name or f"{plugin_name}-Fab-Source.zip"
    zip_path = os.path.join(args.out_dir, zip_name)
    write_zip(stage_dir, plugin_name, files, zip_path)

    with open(zip_path + ".manifest.txt", "w", encoding="utf-8", newline="\n") as mf:
        for rel in files:
            full = os.path.join(stage_dir, rel)
            with open(full, "rb") as f:
                digest = hashlib.sha256(f.read()).hexdigest()
            mf.write(f"{digest}  {os.path.getsize(full):>10}  {rel}\n")

    print(f"Plugin:           {plugin_name}")
    print(f"Files packaged:   {len(files)}")
    print(f"Modules removed:  {', '.join(report['removed_modules']) or '(none)'}")
    for m in report["absent_modules"]:
        print(f"Module not present, nothing to remove: {m}")
    print(f"Files excluded by rule ({len(report['excluded_files'])}):")
    for rel in report["excluded_files"]:
        print(f"  - {rel}")
    print(f"Zip:              {zip_path} ({os.path.getsize(zip_path)} bytes)")

    errors = report["tree_errors"] + check_package(zip_path, plugin_name, excluded_modules)
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a", encoding="utf-8") as s:
            s.write(f"### Fab source package\n\n`{zip_name}`: {len(files)} files. "
                    f"Modules removed: {', '.join(report['removed_modules']) or 'none'}. "
                    f"Files excluded by rule: {len(report['excluded_files'])}.\n\n")
            s.write("Package checks: " + ("**passed**" if not errors else "**failed**") + "\n\n")
            for e in errors:
                s.write(f"- {e}\n")
    if errors:
        for e in errors:
            print(f"::error::fab-package: {e}")
        return 1
    print("[OK] Package checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
