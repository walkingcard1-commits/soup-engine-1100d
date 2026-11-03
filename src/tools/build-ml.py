#!/usr/bin/env python3
"""
Build a Magic Lantern autoexec with the Soup Film menu.

The Magic Lantern tree is not part of this repo. This checks out the
pinned commit, copies integrations/magiclantern/common and then the body's
own directory over it, and builds that body's platform. The engine sources
stay in src/engine and the platform Makefile finds them from that checkout.

  python3 tools/build-ml.py --model 50D
  python3 tools/build-ml.py --model 1100D --card I:\\

Bodies: 50D (firmware 1.0.9) and 1100D (firmware 1.0.5).

Do not use tools/patch-film.py for either. That plants a GUI hook the 50D
does not survive. See ml/platforms/50D.109/PITFALLS.md.
"""

import argparse
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.dirname(HERE)
ROOT = os.path.dirname(SRC)
OVERLAYS = os.path.join(ROOT, "integrations", "magiclantern")
COMMON = os.path.join(OVERLAYS, "common")
ML = os.path.join(SRC, "vendor", "magiclantern")
PIN_FILE = os.path.join(COMMON, "SOUP-PIN.txt")

# platform directory in the Magic Lantern tree, and the name of the symbol
# file it builds. The sym file must sit in ML/modules on the card.
#
# "ml_top" is the highest address Magic Lantern's image may reach, where the
# body loads it into a fixed region it carves out of Canon's memory pool. The
# 1100D has one (0xC80100 to 0xD00000); an image whose BSS runs past it
# overwrites Canon's data and the camera does not boot. The 50D sizes its
# region from the image, so it has none.
MODELS = {
    "50D":   {"platform": "50D.109",   "sym": "50D_109.sym", "ml_top": None},
    "1100D": {"platform": "1100D.105", "sym": "t3_105.sym",  "ml_top": 0xD00000},
}
MIN_SPARE = 4096
REMOTE = "https://github.com/reticulatedpines/magiclantern_simplified"


def die(msg):
    sys.stderr.write("build-ml: %s\n" % msg)
    raise SystemExit(1)


def run(cmd, cwd=None, env=None):
    sys.stderr.write("+ %s\n" % " ".join(cmd))
    r = subprocess.call(cmd, cwd=cwd, env=env)
    if r != 0:
        die("command failed (%d)" % r)


def pin_commit():
    if not os.path.isfile(PIN_FILE):
        die("missing %s" % PIN_FILE)
    for line in open(PIN_FILE, "r", encoding="utf-8"):
        line = line.strip()
        if line.lower().startswith("commit "):
            return line.split()[1]
    die("no Commit line in %s" % PIN_FILE)


def git(args, cwd):
    run(["git"] + args, cwd=cwd)


def ensure_tree(commit):
    if not os.path.isdir(os.path.join(ML, ".git")):
        os.makedirs(os.path.dirname(ML), exist_ok=True)
        run(["git", "clone", REMOTE, ML])
        git(["checkout", "--detach", commit], ML)
    elif not os.path.isfile(os.path.join(ML, "src", "film_menu.c")):
        git(["fetch", "--depth", "1", "origin", commit], ML)
        git(["checkout", "--detach", "FETCH_HEAD"], ML)


def copy_tree(overlay):
    if not os.path.isdir(overlay):
        die("missing overlay %s" % overlay)
    for dirpath, _dirs, files in os.walk(overlay):
        rel = os.path.relpath(dirpath, overlay)
        for name in files:
            if rel == "." and name == "SOUP-PIN.txt":
                continue
            src = os.path.join(dirpath, name)
            dst = os.path.join(ML, rel, name) if rel != "." else os.path.join(ML, name)
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            shutil.copy2(src, dst)


def copy_overlay(platform):
    """Shared files first, then the body's own, so a body can override."""
    copy_tree(COMMON)
    copy_tree(os.path.join(OVERLAYS, platform))


def arm_binpath():
    env = os.environ.get("ARM_BINPATH")
    if env and os.path.isfile(os.path.join(env, "arm-none-eabi-gcc")):
        return env
    candidates = [
        r"C:\PROGRA~2\ARMGNU~1\14EFD8~1.2RE\bin",
        "/usr/bin",
    ]
    for c in candidates:
        if os.path.isfile(os.path.join(c, "arm-none-eabi-gcc")) or \
           os.path.isfile(os.path.join(c, "arm-none-eabi-gcc.exe")):
            return c
    die("set ARM_BINPATH to the directory that contains arm-none-eabi-gcc")


def build(m):
    plat = os.path.join(ML, "platform", m["platform"])
    env = os.environ.copy()
    arm = arm_binpath()
    extra = [arm]
    git_usr = r"C:\Program Files\Git\usr\bin"
    if os.path.isdir(git_usr):
        extra.append(git_usr)
    env["PATH"] = os.pathsep.join(extra + [env.get("PATH", "")])
    run([
        "make",
        "ARM_BINPATH=" + arm.replace("\\", "/"),
        "PYTHON=" + sys.executable.replace("\\", "/"),
        "build/autoexec.bin",
        "build/" + m["sym"],
    ], cwd=plat, env=env)
    auto = os.path.join(plat, "build", "autoexec.bin")
    sym = os.path.join(plat, "build", m["sym"])
    if not os.path.isfile(auto) or not os.path.isfile(sym):
        die("build did not produce autoexec.bin and %s" % m["sym"])
    check_fit(m, plat, arm)
    return auto, sym


def check_fit(m, plat, arm):
    """Refuse an image that will not fit the region the body gives it."""
    top = m["ml_top"]
    if top is None:
        return
    nm = os.path.join(arm, "arm-none-eabi-nm")
    if not os.path.isfile(nm) and os.path.isfile(nm + ".exe"):
        nm += ".exe"
    out = subprocess.check_output([nm, os.path.join(plat, "build", "magiclantern")],
                                  universal_newlines=True)
    end = None
    for line in out.splitlines():
        f = line.split()
        if len(f) == 3 and f[2] == "_bss_end":
            end = int(f[0], 16)
    if end is None:
        die("could not find _bss_end in the linked image")
    spare = top - end
    if spare < 0:
        die("Magic Lantern is %d bytes too big for this body: its BSS ends at "
            "0x%X and the region ends at 0x%X. It would not boot. Take "
            "features out in platform/%s/features.h." %
            (-spare, end, top, m["platform"]))
    sys.stderr.write("fits: %d bytes spare below 0x%X\n" % (spare, top))
    if spare < MIN_SPARE:
        sys.stderr.write("build-ml: warning: under %d bytes spare; a different "
                         "compiler version could tip this over\n" % MIN_SPARE)


def copy_card(card, auto, sym, symname):
    root = card.rstrip("\\/")
    if not os.path.isdir(root):
        die("card not found: %s" % card)
    shutil.copy2(auto, os.path.join(root, "AUTOEXEC.BIN"))
    mod = os.path.join(root, "ML", "modules")
    os.makedirs(mod, exist_ok=True)
    shutil.copy2(sym, os.path.join(mod, symname))
    film = os.path.join(root, "ML", "FILM")
    os.makedirs(film, exist_ok=True)
    stocks = os.path.join(SRC, "stocks")
    for name in os.listdir(stocks):
        if name.endswith(".fs") or name == "INDEX.TXT":
            shutil.copy2(os.path.join(stocks, name), os.path.join(film, name))
    sys.stderr.write("copied AUTOEXEC.BIN, %s, and ML/FILM to %s\n" % (symname, root))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--model", required=True, choices=sorted(MODELS),
                    help="which camera body to build for")
    ap.add_argument("--card", help="copy AUTOEXEC.BIN, the sym, and ML/FILM here")
    args = ap.parse_args()
    m = MODELS[args.model]
    commit = pin_commit()
    ensure_tree(commit)
    copy_overlay(m["platform"])
    auto, sym = build(m)
    if args.card:
        copy_card(args.card, auto, sym, m["sym"])
    else:
        sys.stderr.write("built %s\nbuilt %s\n" % (auto, sym))


if __name__ == "__main__":
    main()
