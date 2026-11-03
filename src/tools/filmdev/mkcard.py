#!/usr/bin/env python3
"""
Lay out a fake card for filmdevtest: ML/FILM from src/stocks, ML/SE.CFG, and
DCIM/100CANON/IMG_0001.CR2 whose first IFD carries a JPEG preview of the
given size. The raw data after it is filler; film_develop.c never reads it.

    mkcard.py <card-dir> <width> <height> [stock]
"""
import io, os, shutil, struct, sys
import numpy as np
from PIL import Image

card, W, H = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
stock = sys.argv[4] if len(sys.argv) > 4 else "PROVIA"
here = os.path.dirname(os.path.abspath(__file__))
stocks = os.path.join(here, "..", "..", "stocks")

film = os.path.join(card, "ML", "FILM")
dcim = os.path.join(card, "DCIM", "100CANON")
shutil.rmtree(card, ignore_errors=True)
os.makedirs(film); os.makedirs(dcim)
for n in os.listdir(stocks):
    if n.endswith(".fs") or n == "INDEX.TXT":
        shutil.copy(os.path.join(stocks, n), film)
open(os.path.join(card, "ML", "SE.CFG"), "w").write("FILM ON\nDELETE OFF\nSTOCK %s\n" % stock)

# a scene with something in every part of the frame: sky gradient, ground,
# a ramp, coloured blocks, a little noise. Not a photograph, but it has
# edges, flats and gradients for the JPEG and the look to chew on.
y, x = np.mgrid[0:H, 0:W].astype(np.float32)
img = np.zeros((H, W, 3), np.float32)
sky = (y / H)
img[..., 0] = 90 + 110 * sky
img[..., 1] = 140 + 70 * sky
img[..., 2] = 230 - 90 * sky
ground = y > H * 0.62
img[ground] = np.stack([60 + 40 * (x / W), 90 + 30 * np.sin(x / 90), 40 + 0 * x], -1)[ground]
for i, c in enumerate([(220, 40, 40), (40, 200, 60), (50, 70, 220), (240, 220, 60), (128, 128, 128)]):
    x0 = int(W * (0.08 + i * 0.18)); img[int(H*0.15):int(H*0.35), x0:x0 + int(W*0.12)] = c
img[int(H*0.7):int(H*0.8), :] = (np.linspace(0, 255, W)[None, :, None] * np.ones(3)).astype(np.float32)
img += np.random.RandomState(1).normal(0, 3, img.shape)
jb = io.BytesIO()
Image.fromarray(np.clip(img, 0, 255).astype(np.uint8)).save(jb, "JPEG", quality=92, subsampling=1)
jpeg = jb.getvalue()

ifd = 16
nent = 5
strip = 128
ents = [(0x100, 3, W), (0x101, 3, H), (0x103, 3, 6), (0x111, 4, strip), (0x117, 4, len(jpeg))]
buf = bytearray(strip)
buf[0:4] = b"II*\x00"
struct.pack_into("<I", buf, 4, ifd)
buf[8:12] = b"CR\x02\x00"
struct.pack_into("<I", buf, 12, 0)
struct.pack_into("<H", buf, ifd, nent)
for i, (tag, typ, val) in enumerate(ents):
    struct.pack_into("<HHII", buf, ifd + 2 + i * 12, tag, typ, 1, val)
out = bytes(buf) + jpeg
out += np.random.RandomState(2).bytes(max(0, 8 * 1024 * 1024 - len(out)))
open(os.path.join(dcim, "IMG_0001.CR2"), "wb").write(out)
print("card ready: preview %dx%d, %d bytes of JPEG, CR2 %d bytes" % (W, H, len(jpeg), len(out)))
