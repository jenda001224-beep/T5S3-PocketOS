#!/usr/bin/env python3
"""
Download offline map tiles for PocketOS Maps app.
Saves as /pocket/maps/{zoom}/{x}/{y}.raw on SD card.

Each .raw file = 256×256 pixels, 1 byte per pixel, value 0-15 (grayscale).

Usage:
    python3 download_maps.py --lat 50.075 --lng 14.437 --radius 0.1 --zoom 12 13 14 --out ./maps

Then copy the maps/ folder to the SD card under /pocket/maps/
"""

import os
import math
import struct
import argparse
import urllib.request
from PIL import Image

TILE_SERVER = "https://tile.openstreetmap.org/{z}/{x}/{y}.png"

def lat_lng_to_tile(lat, lng, zoom):
    n = 2 ** zoom
    x = int((lng + 180) / 360 * n)
    lat_r = math.radians(lat)
    y = int((1 - math.log(math.tan(lat_r) + 1/math.cos(lat_r)) / math.pi) / 2 * n)
    return x, y

def download_tile(z, x, y, out_dir):
    os.makedirs(f"{out_dir}/{z}/{x}", exist_ok=True)
    path = f"{out_dir}/{z}/{x}/{y}.raw"
    if os.path.exists(path):
        return

    url = TILE_SERVER.format(z=z, x=x, y=y)
    headers = {"User-Agent": "PocketOS/1.0 (offline tile cache)"}
    req = urllib.request.Request(url, headers=headers)
    try:
        with urllib.request.urlopen(req, timeout=10) as r:
            data = r.read()
    except Exception as e:
        print(f"  SKIP {z}/{x}/{y}: {e}")
        return

    # Convert PNG to 256×256 grayscale 4bpp (stored as 1 byte/pixel, 0-15)
    from io import BytesIO
    img = Image.open(BytesIO(data)).convert("L").resize((256, 256))
    pixels = list(img.getdata())
    raw = bytes(p >> 4 for p in pixels)  # 8bpp → 4bpp (0-15)
    with open(path, "wb") as f:
        f.write(raw)
    print(f"  OK  {z}/{x}/{y}")

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--lat",    type=float, default=50.075)
    p.add_argument("--lng",    type=float, default=14.437)
    p.add_argument("--radius", type=float, default=0.05,
                   help="Degrees radius around center point")
    p.add_argument("--zoom",   type=int, nargs="+", default=[12, 13, 14])
    p.add_argument("--out",    default="./maps_out")
    args = p.parse_args()

    for zoom in args.zoom:
        tx0, ty0 = lat_lng_to_tile(args.lat + args.radius, args.lng - args.radius, zoom)
        tx1, ty1 = lat_lng_to_tile(args.lat - args.radius, args.lng + args.radius, zoom)
        total = (tx1 - tx0 + 1) * (ty1 - ty0 + 1)
        print(f"Zoom {zoom}: {tx1-tx0+1}×{ty1-ty0+1} = {total} tiles")
        for tx in range(tx0, tx1 + 1):
            for ty in range(ty0, ty1 + 1):
                download_tile(zoom, tx, ty, args.out)

    print(f"\nDone. Copy {args.out}/ to SD card as /pocket/maps/")

if __name__ == "__main__":
    main()
