"""Cut the loops of a reference folder into single hits and write a catalogue.

    python slice.py <reference folder> <output folder>

Every hit is saved as a WAV in the output folder (keep it outside the repository or in an ignored
folder) and described in catalogue.csv: tonality, pitch at the start and after 100 ms, time to fall
20 dB, spectral centroid and a rough class (kick/tom, tonal, noise, metal)."""
import csv
import glob
import os
import sys

import numpy as np
from scipy.io import wavfile

from common import SR, Features, load, pitch_track, slice_hits


def describe(x):
    f = Features(x)
    t, fr = pitch_track(x)
    start = np.median(fr[t < 0.006]) if np.any(t < 0.006) else np.nan
    body = np.median(fr[(t > 0.08) & (t < 0.15)]) if np.any((t > 0.08) & (t < 0.15)) else np.nan
    level = f.level
    peak = int(np.argmax(level))
    below = np.where(level[peak:] < -20)[0]
    t20 = Features.LEVEL_GRID[peak + below[0]] * 1000 if len(below) else 600.0
    spec = np.abs(np.fft.rfft(x[:int(0.05 * SR)]))
    freqs = np.fft.rfftfreq(int(0.05 * SR), 1 / SR)
    centroid = float(np.sum(freqs * spec) / (np.sum(spec) + 1e-12))
    sweep = np.log2(start / body) if start == start and body == body and body > 0 else 0.0
    if f.tonality < 0.5:
        kind = 'noise'
    elif centroid > 2500 and f.tonality < 0.8:
        kind = 'metal'
    elif body == body and body < 160 and sweep > 0.8:
        kind = 'kick'
    else:
        kind = 'tonal'
    return dict(tonality=round(f.tonality, 3), f_start=round(float(start), 1), f_body=round(float(body), 1),
                sweep_oct=round(float(sweep), 2), t20_ms=round(float(t20), 1), centroid=round(centroid), kind=kind)


def main(reference, out):
    os.makedirs(out, exist_ok=True)
    rows = []
    for path in sorted(glob.glob(os.path.join(reference, '**', '*.wav'), recursive=True)):
        if os.path.abspath(out) in os.path.abspath(path):
            continue
        x = load(path, trim=False, normalise=False)
        hits = slice_hits(x) if len(x) > SR else [(0, len(x))]
        for k, (a, b) in enumerate(hits):
            seg = x[a:min(b, a + int(1.0 * SR))]
            if len(seg) < int(0.05 * SR) or np.abs(seg).max() < 0.02:
                continue
            seg = seg / np.abs(seg).max()
            name = f"{os.path.splitext(os.path.basename(path))[0].replace(' ', '_')}_{k:02d}.wav"
            wavfile.write(os.path.join(out, name), SR, (seg * 32000).astype(np.int16))
            rows.append(dict(file=name, source=os.path.basename(path), at_ms=round(a / SR * 1000), **describe(seg)))
    with open(os.path.join(out, 'catalogue.csv'), 'w', newline='') as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    kinds = {}
    for r in rows:
        kinds[r['kind']] = kinds.get(r['kind'], 0) + 1
    print(f"{len(rows)} hits: " + ", ".join(f"{k} {v}" for k, v in sorted(kinds.items())))


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
