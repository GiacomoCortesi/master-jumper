"""Generates SourceArt/Textures/T_CorridorWall.png (no third-party deps).

One 400 x 330 cm corridor wall panel, top row = ceiling line (matches SM_CorridorWallPanel UVs, V runs top -> bottom):
  cornice line, warm off-white upper wall with faint speckle, dado rail at 100 cm, blue-grey wainscot,
  dark baseboard, darker seams at both vertical edges (panel joins).
Run:  python3 Tools/gen_wall_texture.py
"""
import os, random, struct, zlib

W = H = 512
WALL_CM = 330.0


def v_of(height_cm):
    """Texture row for a height above the floor."""
    return int(round((1.0 - height_cm / WALL_CM) * (H - 1)))


def png(path, pixels):
    raw = b''.join(b'\x00' + bytes(row) for row in pixels)
    def chunk(tag, data):
        c = struct.pack('>I', len(data)) + tag + data
        return c + struct.pack('>I', zlib.crc32(tag + data) & 0xffffffff)
    data = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', W, H, 8, 2, 0, 0, 0))
    data += chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b'')
    with open(path, 'wb') as f:
        f.write(data)


def main():
    rng = random.Random(1993)
    upper = (214, 208, 192)
    wainscot = (96, 112, 122)
    rail = (60, 64, 68)
    base = (38, 38, 40)
    cornice = (170, 166, 156)
    seam = 0.78

    rows_rail = range(v_of(103), v_of(97) + 1)
    rows_base = range(v_of(12), H)
    rows_cornice = range(0, v_of(325) + 1)

    pixels = []
    for y in range(H):
        if y in rows_cornice:
            color = cornice
        elif y in rows_rail:
            color = rail
        elif y in rows_base:
            color = base
        elif y > v_of(100):
            color = wainscot
        else:
            color = upper
        row = []
        for x in range(W):
            n = rng.uniform(-6, 6)
            # faint vertical paint streaks on the upper wall, subtle grooves on the wainscot
            if color is upper:
                n += 3.0 * ((x * 7919 % 97) / 97.0 - 0.5)
            elif color is wainscot and x % 64 in (0, 1):
                n -= 14
            r, g, b = (max(0, min(255, int(c + n))) for c in color)
            if x < 3 or x >= W - 3:
                r, g, b = int(r * seam), int(g * seam), int(b * seam)
            row += [r, g, b]
        pixels.append(row)

    out = os.path.join(os.path.dirname(__file__), '..', 'SourceArt', 'Textures', 'T_CorridorWall.png')
    os.makedirs(os.path.dirname(out), exist_ok=True)
    png(os.path.normpath(out), pixels)
    print('wrote', os.path.normpath(out))


if __name__ == '__main__':
    main()
