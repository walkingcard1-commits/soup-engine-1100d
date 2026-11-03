#!/usr/bin/env python3
"""
Check what filmdevtest wrote. check.py <card-dir> <width> <height>
FLM_0001.JPG must be exactly that size, 4:2:2, and the raw's embedded
preview must have been replaced by the same picture with its tags updated.
"""
import io, os, struct, sys
from PIL import Image, JpegImagePlugin

card, W, H = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
d = os.path.join(card, "DCIM", "100CANON")
ok = True
def check(c, what):
    global ok
    print("  [%s] %s" % ("pass" if c else "FAIL", what)); ok = ok and c

jb = open(os.path.join(d, "FLM_0001.JPG"), "rb").read()
im = Image.open(io.BytesIO(jb)); im.load()
check(im.size == (W, H), "FLM_0001.JPG is %dx%d (got %dx%d)" % (W, H, *im.size))
check(JpegImagePlugin.get_sampling(im) == 1, "chroma is 4:2:2")
check(not os.path.exists(os.path.join(d, "FLM_0001.TMP")), "no .TMP left behind")

cr2 = open(os.path.join(d, "IMG_0001.CR2"), "rb").read()
ifd = struct.unpack_from("<I", cr2, 4)[0]
n = struct.unpack_from("<H", cr2, ifd)[0]
t = {}
for i in range(n):
    tag, typ, cnt, val = struct.unpack_from("<HHII", cr2, ifd + 2 + i * 12)
    t[tag] = val & 0xffff if typ == 3 else val
check((t[0x100], t[0x101]) == (W, H), "CR2 preview tags say %dx%d (got %dx%d)" % (W, H, t[0x100], t[0x101]))
pv = cr2[t[0x111]:t[0x111] + t[0x117]]
check(pv == jb, "CR2 preview is the same bytes as FLM_0001.JPG")
Image.open(io.BytesIO(pv)).load()
sys.exit(0 if ok else 1)
