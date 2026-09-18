#!/usr/bin/env python3
"""App-Symbol: die Strasse.

Aufruf: make_app_icon.py <zielordner>          -> system_icon.png (25x25)
        make_app_icon.py --store <zielordner>  -> icon-144.png, icon-48.png

Eine Fahrbahn in der Flucht, mit Mittellinie: zwei Raender, die nach oben
zusammenlaufen, und drei Striche dazwischen. Kein Pfeil - der steht schon auf
dem Schirm der App, und zweimal dasselbe Zeichen sagt nichts zweimal.

MASSSTAB IST DAS SYSTEMSYMBOL, wie bei den Geschwistern: die Uhr-Kachel von
"Watchfaces" im Starter wurde Punkt fuer Punkt nachgemessen - 24 von 25 Punkten
hoch, Linien 2 bis 3 Punkte stark, rund 180 schwarze Punkte. Eine duennere
Linie sieht daneben aus wie ein Versehen.

NUR LINIEN, KEINE FLAECHE, und keine ~bw-Fassung. Der Starter zeichnet Symbole
einfarbig; eine farbige Flaeche kam dort als grauer Fleck heraus.

DER STORE NIMMT NICHTS AUS DER .pbw. Im Entwicklerportal liegen zwei eigene
Bilder, `icon_large` und `icon_small`; angefordert werden sie in festen Massen
(gross 80 und 144, klein 28 und 48), jeweils mit `exact` in der Adresse, also
erzwungen statt eingepasst. Darum eine gefuellte Kachel: das grosse Symbol legt
der Store fuer sein Teilen-Bild durch eine abgerundete Maske, und ueber einer
durchsichtigen Strichzeichnung taete die nichts.

DIE FORM STEHT NUR EINMAL DA. Alle Masse gelten auf einem Raster von 25
Punkten und werden mit s hochgerechnet; mit s = 1 kommt das Uhr-Symbol heraus.
"""
import math
import os
import struct
import sys
import zlib

RASTER = 25                      # Bezugsraster, auf dem alle Masse gelten
SS = 4                           # Ueberabtastung je Achse
LINE = 2.4                       # Strichstaerke in Punkten
LW = LINE / 2.0 - 0.15           # halbe Strichstaerke, minus Rundungsluft

# Die beiden Raender. Oben eng, unten weit - das ist die ganze Flucht.
OBEN, UNTEN = 2.0, 23.0
X_OL, X_OR = 10.5, 14.5          # oben links und rechts
X_UL, X_UR = 1.0, 24.0           # unten links und rechts

# Die Mittellinie: drei Striche, nach unten laenger und dicker werdend, damit
# sie in derselben Flucht liegen wie die Raender.
#
# KURZ UND WEIT AUSEINANDER. Ein erster Versuch mit langen Strichen und engen
# Luecken las sich als Kette: bei 25 Punkten zaehlt der Zwischenraum mehr als
# der Strich, sonst laufen drei Markierungen zu einer Linie zusammen.
MITTE = 12.5
STRICHE = ((3.5, 5.5, 0.8), (10.0, 13.0, 1.0), (18.0, 22.5, 1.25))

# Store-Kachel. Nachgeschlagen in gcolor_definitions.h des SDK.
GRUND = (0x00, 0x55, 0xAA)       # GColorCobaltBlue
STRICH = (0xFF, 0xFF, 0xFF)      # weiss
FUELL = 0.72
STORE_GROESSEN = (144, 48)


def png(path, w, h, rows):
    """Minimaler PNG-Schreiber, 8 Bit RGBA, ohne Fremdbibliothek."""
    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xffffffff)
    raw = b"".join(b"\x00" + bytes(r) for r in rows)
    out = b"\x89PNG\r\n\x1a\n"
    out += chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
    out += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(out)


def seg(x, y, ax, ay, bx, by, r):
    """Abstand zur Strecke - so ist die Linie auch an den Enden gleich stark."""
    dx, dy = bx - ax, by - ay
    L2 = dx * dx + dy * dy
    t = 0.0 if L2 == 0 else ((x - ax) * dx + (y - ay) * dy) / L2
    t = max(0.0, min(1.0, t))
    return math.hypot(x - (ax + t * dx), y - (ay + t * dy)) <= r


def raster(test, n):
    """Vierfach ueberabtasten, bei halber Deckung schneiden. Harte Kanten."""
    grid = []
    for py in range(n):
        row = []
        for px in range(n):
            hits = 0
            for sy in range(SS):
                for sx in range(SS):
                    if test(px + (sx + 0.5) / SS, py + (sy + 0.5) / SS):
                        hits += 1
            row.append(hits * 2 >= SS * SS)
        grid.append(row)
    return grid


def pruefer(s):
    """Der Formtest, auf den Massstab s gebracht."""
    lw = LW * s

    def inside(x, y):
        if seg(x, y, X_OL * s, OBEN * s, X_UL * s, UNTEN * s, lw):
            return True
        if seg(x, y, X_OR * s, OBEN * s, X_UR * s, UNTEN * s, lw):
            return True
        for (y0, y1, dick) in STRICHE:
            if seg(x, y, MITTE * s, y0 * s, MITTE * s, y1 * s, lw * dick):
                return True
        return False
    return inside


def schreibe_uhr(dest):
    n = RASTER
    grid = raster(pruefer(1.0), n)
    rows = []
    for y in range(n):
        r = []
        for x in range(n):
            r += [0, 0, 0, 255] if grid[y][x] else [0, 0, 0, 0]
        rows.append(r)
    png(os.path.join(dest, "system_icon.png"), n, n, rows)
    punkte = sum(1 for r in grid for v in r if v)
    ys = [y for y in range(n) if any(grid[y])]
    print("system_icon.png: %d Punkte schwarz, %d hoch (Vorbild: 180 / 24)"
          % (punkte, (ys[-1] - ys[0] + 1) if ys else 0))


def schreibe_store(dest):
    for gross in STORE_GROESSEN:
        innen = int(round(gross * FUELL))
        grid = raster(pruefer(innen / float(RASTER)), innen)
        rand = (gross - innen) // 2
        rows = []
        for y in range(gross):
            r = []
            for x in range(gross):
                iy, ix = y - rand, x - rand
                treffer = 0 <= iy < innen and 0 <= ix < innen and grid[iy][ix]
                farbe = STRICH if treffer else GRUND
                r += [farbe[0], farbe[1], farbe[2], 255]
            rows.append(r)
        name = "icon-%d.png" % gross
        png(os.path.join(dest, name), gross, gross, rows)
        print("%s: Kachel %s, Strasse weiss" % (name, "#%02X%02X%02X" % GRUND))


def main():
    args = sys.argv[1:]
    store = "--store" in args
    if store:
        args.remove("--store")
    dest = args[0] if args else ("store" if store else "resources/images")
    os.makedirs(dest, exist_ok=True)
    if store:
        schreibe_store(dest)
    else:
        schreibe_uhr(dest)


if __name__ == "__main__":
    main()
