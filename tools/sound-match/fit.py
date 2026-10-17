"""Match PS-8 to reference hits: for each hit, search the PS-8 settings that sound closest, then show what
is left over on average (a systematic difference points at the engine, not at a preset).

    python fit.py <PS8Tests executable> <hits folder with catalogue.csv> [per-kind] [evaluations] > report.txt

per-kind: how many representative hits to fit per class (default 6); results go to fits.json next to
the hits. FIT_KINDS=tonal,metal limits the classes, FIT_SKIP=Misc leaves sources out (e.g. loops
recorded with effects)."""
import csv
import json
import os
import sys

import numpy as np
from scipy.cluster.vq import kmeans2
from scipy.optimize import minimize

from common import Features, Renderer, load

# continuous parameters (plain values) and their ranges, per class of hit
SPACES = {
    'kick':  ['vco1Freq', 'vco1Eg', 'pitchDecay', 'vcaDecay', 'cutoff', 'resonance', 'vcfEg', 'vcfDecay'],
    'tonal': ['vco1Freq', 'vco1Eg', 'vco1Level', 'vco2Freq', 'vco2Eg', 'vco2Level', 'fmAmount',
              'pitchDecay', 'vcaDecay', 'cutoff', 'resonance', 'vcfEg', 'vcfDecay'],
    'noise': ['noiseLevel', 'vco1Freq', 'vco1Level', 'cutoff', 'resonance', 'vcfEg', 'vcfDecay', 'vcaDecay'],
    'metal': ['vco1Freq', 'vco2Freq', 'vco1Level', 'vco2Level', 'fmAmount', 'noiseLevel', 'cutoff',
              'resonance', 'vcfEg', 'vcfDecay', 'vcaDecay'],
}
RANGES = dict(vco1Freq=(0, 1), vco2Freq=(0, 1), vco1Eg=(-1, 1), vco2Eg=(-1, 1), vco1Level=(0, 1), vco2Level=(0, 1),
              fmAmount=(0, 1), noiseLevel=(0, 1), pitchDecay=(0, 0.9), vcaDecay=(0, 0.95), cutoff=(0, 1),
              resonance=(0, 0.9), vcfEg=(-1, 1), vcfDecay=(0, 0.9))
# switch settings tried for each class
SWITCHES = {
    'kick':  [dict(vco1Square=1), dict(vco1Square=0)],
    'tonal': [dict(vco1Square=0, vco2Square=1), dict(vco1Square=1, vco2Square=1), dict(vco1Square=0, vco2Square=0)],
    'noise': [dict(highPass=1, noisePink=0), dict(highPass=0, noisePink=0), dict(highPass=1, noisePink=1)],
    'metal': [dict(vco1Square=1, vco2Square=1, highPass=1), dict(vco1Square=1, vco2Square=1, highPass=0)],
}
FIXED = {
    'kick':  dict(vco2Level=0, noiseLevel=0, vco1Level=1, vco2Eg=0),
    'tonal': dict(noiseLevel=0),
    'noise': dict(vco2Level=0, vco1Eg=0, vco2Eg=0, pitchDecay=0.2),
    'metal': dict(vco1Eg=0, vco2Eg=0),
}


def start_point(kind, row):
    body = float(row['f_body']) if row['f_body'] not in ('nan', '') else 200.0
    freq = np.clip(np.log2(max(body, 9) / 8) / 10, 0, 1)
    t20 = float(row['t20_ms']) / 1000
    vca = np.clip(np.log(max(t20 * 3, 0.004) / 0.003) / np.log(10 / 0.003), 0.05, 0.9)
    sweep = max(0.0, float(row['sweep_oct']))
    start = dict(vco1Freq=freq, vco2Freq=min(1, freq + 0.08), vco1Eg=np.sqrt(min(sweep, 5) / 5), vco2Eg=0.1,
                 vco1Level=0.8, vco2Level=0.3, fmAmount=0.05, noiseLevel=0.7, pitchDecay=0.45, vcaDecay=vca,
                 cutoff=0.6, resonance=0.2, vcfEg=0.3, vcfDecay=0.35)
    if kind == 'kick':
        start.update(cutoff=0.22, vcfEg=0.7, vcfDecay=0.12)
    if kind == 'noise':
        start.update(cutoff=0.65, vco1Level=0.1)
    if kind == 'metal':
        start.update(vco1Freq=0.75, vco2Freq=0.8, fmAmount=0.6, noiseLevel=0.3, cutoff=0.7, vco1Level=0.7, vco2Level=0.7)
    return start


def fit(renderer, kind, target, row, evaluations):
    names = SPACES[kind]
    lo = np.array([RANGES[n][0] for n in names])
    hi = np.array([RANGES[n][1] for n in names])
    best = None

    for switches in SWITCHES[kind]:
        fixed = dict(FIXED[kind], **switches)

        def options(p):
            p = np.clip(p, lo, hi)
            return [f'{k}={v}' for k, v in fixed.items()] + [f'{n}={v:.4f}' for n, v in zip(names, p)]

        def loss(p):
            d, _ = Features(renderer.render(['Init'] + options(p))).distance(target)
            return d + 10 * np.sum(np.maximum(0, p - hi) + np.maximum(0, lo - p))

        x0 = np.array([start_point(kind, row)[n] for n in names])
        # Powell copes better than Nelder-Mead with many settings at once (line searches per setting)
        res = minimize(loss, x0, method='Powell', bounds=list(zip(lo, hi)),
                       options=dict(maxfev=evaluations, xtol=1e-3, ftol=1e-4))
        if best is None or res.fun < best[0]:
            best = (res.fun, np.clip(res.x, lo, hi), fixed)

    loss_value, p, fixed = best
    settings = dict(fixed, **{n: round(float(v), 4) for n, v in zip(names, p)})
    rendered = Features(renderer.render(['Init'] + [f'{k}={v}' for k, v in settings.items()]))
    return loss_value, settings, rendered


def representatives(rows, per_kind):
    picked = {}
    for kind in SPACES:
        group = [r for r in rows if r['kind'] == kind]
        if not group:
            continue
        def num(r, k, default):
            v = r[k]
            return float(v) if v not in ('nan', '') else default
        data = np.array([[np.log2(max(num(r, 'f_body', 200), 10)), num(r, 'sweep_oct', 0),
                          np.log2(max(num(r, 't20_ms', 100), 5)), np.log2(max(num(r, 'centroid', 1000), 50)),
                          num(r, 'tonality', 0.5)] for r in group])
        k = min(per_kind, len(group))
        scale = data.std(axis=0) + 1e-9
        centroids, labels = kmeans2((data - data.mean(axis=0)) / scale, k, seed=1, minit='++')
        for c in range(k):
            members = np.where(labels == c)[0]
            if len(members):
                centre = centroids[c]
                closest = members[np.argmin(np.sum(((data[members] - data.mean(axis=0)) / scale - centre) ** 2, axis=1))]
                picked.setdefault(kind, []).append(group[closest])
    return picked


def main(exe, hits, per_kind=6, evaluations=350, kinds=None, skip=None):
    rows = list(csv.DictReader(open(os.path.join(hits, 'catalogue.csv'))))
    if skip:
        rows = [r for r in rows if not any(s in r['source'] for s in skip.split(','))]
    renderer = Renderer(exe)
    results = {}
    for kind, group in representatives(rows, per_kind).items():
        if kinds and kind not in kinds.split(','):
            continue
        level_diff, early_diff, late_diff, parts = [], [], [], []
        for row in group:
            target = Features(load(os.path.join(hits, row['file'])))
            loss_value, settings, rendered = fit(renderer, kind, target, row, evaluations)
            _, part = rendered.distance(target)
            results[row['file']] = dict(kind=kind, loss=round(float(loss_value), 4), settings=settings,
                                        **{k: round(float(v), 4) for k, v in part.items()})
            level_diff.append(target.level - rendered.level)
            early_diff.append(target.spectrum_early - rendered.spectrum_early)
            late_diff.append(target.spectrum_late - rendered.spectrum_late)
            level_diff[-1] = np.where(np.isnan(level_diff[-1]), np.nan, level_diff[-1])
            parts.append(part)
            print(f"{kind:5s} {row['file'][:28]:28s} loss {loss_value:5.3f}  level {np.sqrt(part['level'])*10:4.1f} dB  "
                  f"spectrum {np.sqrt(part['spectrum'])*15:4.1f} dB  pitch {np.sqrt(part['pitch']):.2f} oct", flush=True)
            print("      " + " ".join(f"{k}={v}" for k, v in settings.items()), flush=True)
        grid = Features.LEVEL_GRID
        print(f"\n== {kind}: what is left on average (reference minus PS-8)")
        print("   level dB at   " + " ".join(f"{t*1000:5.0f}" for t in grid[::4]))
        print("                 " + " ".join(f"{v:+5.1f}" for v in np.nanmean(level_diff, axis=0)[::4]))
        print("   spectrum 0-40 ms   (1/3 oct from 40 Hz): " + " ".join(f"{v:+.0f}" for v in np.nanmean(early_diff, axis=0)))
        print("   spectrum 40-250 ms (1/3 oct from 40 Hz): " + " ".join(f"{v:+.0f}" for v in np.nanmean(late_diff, axis=0)) + "\n", flush=True)
    renderer.close()
    with open(os.path.join(hits, 'fits.json'), 'w') as f:
        json.dump(results, f, indent=1)


if __name__ == '__main__':
    # optional: FIT_KINDS=tonal,metal  FIT_SKIP=Misc (sources to leave out, e.g. loops with effects)
    main(sys.argv[1], sys.argv[2], int(sys.argv[3]) if len(sys.argv) > 3 else 6,
         int(sys.argv[4]) if len(sys.argv) > 4 else 350, os.environ.get('FIT_KINDS'), os.environ.get('FIT_SKIP'))
