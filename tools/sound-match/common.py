"""Shared helpers for comparing PS-8 with reference recordings: loading, slicing loops into hits,
sound features (level curve, pitch curve, spectrum) and a renderer that keeps one PS8Tests process
running (PS8Tests --render-stdin)."""
import os
import subprocess
import tempfile

import numpy as np
from scipy.io import wavfile
from scipy.signal import butter, hilbert, sosfiltfilt

SR = 48000


def load(path, trim=True, normalise=True):
    sr, x = wavfile.read(path)
    x = x.astype(np.float64)
    if x.ndim > 1:
        x = x.mean(axis=1)
    x /= 32768.0
    if sr != SR:
        raise ValueError(f"{path}: expected {SR} Hz, got {sr}")
    if trim:
        x = x[onset(x):]
    if normalise:
        x = x / (np.abs(x).max() + 1e-12)
    return x


def onset(x, threshold=0.01):
    return max(0, int(np.argmax(np.abs(x) > threshold * np.abs(x).max())) - 1)


def envelope(x, ms=4.0):
    e = np.abs(hilbert(x))
    n = max(1, int(SR * ms / 1000))
    return np.convolve(e, np.ones(n) / n, mode='same')


def slice_hits(x, min_gap=0.06, rise_db=9.0, floor_db=-30.0):
    """Onsets where the short envelope jumps by rise_db within 5 ms; returns (start, end) sample pairs."""
    e = envelope(x, 1.5)
    db = 20 * np.log10(e / (e.max() + 1e-12) + 1e-9)
    d = int(0.005 * SR)
    rise = db[d:] - db[:-d]
    idx = np.where((rise > rise_db) & (db[d:] > floor_db))[0] + d
    onsets, last = [], -10 * SR
    for i in idx:
        if i - last > min_gap * SR:
            onsets.append(max(0, i - d))
            last = i
    return list(zip(onsets, onsets[1:] + [len(x)]))


def pitch_track(x, fmax=1500.0):
    """(time, frequency) per period from rising zero crossings of a low-passed copy"""
    y = sosfiltfilt(butter(4, fmax, 'low', fs=SR, output='sos'), x)
    s = np.signbit(y)
    r = np.where(s[:-1] & ~s[1:])[0]
    if len(r) < 3:
        return np.array([]), np.array([])
    pos = r + y[r] / (y[r] - y[r + 1])
    return (pos[:-1] + pos[1:]) / 2 / SR, SR / np.diff(pos)


def third_octave(seg, lo=40.0, octaves=8.0):
    """level per third-octave band in dB, normalised to the loudest band"""
    if len(seg) < 64:
        seg = np.pad(seg, (0, 64 - len(seg)))
    spec = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 4 * len(seg))) ** 2
    freqs = np.fft.rfftfreq(4 * len(seg), 1 / SR)
    edges = lo * 2 ** np.arange(0, octaves + 1e-9, 1 / 3)
    bands = np.array([spec[(freqs >= a) & (freqs < b)].mean() if np.any((freqs >= a) & (freqs < b)) else 1e-20
                      for a, b in zip(edges[:-1], edges[1:])])
    return np.clip(10 * np.log10(bands / (bands.max() + 1e-20) + 1e-12), -60, 0)


def tonality(seg):
    """0 = noise, 1 = pure tone (1 - spectral flatness over 40 Hz … 12 kHz)"""
    spec = np.abs(np.fft.rfft(seg * np.hanning(len(seg)))) + 1e-12
    freqs = np.fft.rfftfreq(len(seg), 1 / SR)
    spec = spec[(freqs > 40) & (freqs < 12000)]
    return 1.0 - float(np.exp(np.mean(np.log(spec))) / np.mean(spec))


class Features:
    """What a hit sounds like, on fixed grids so two hits can be compared point by point."""

    LEVEL_GRID = np.concatenate([np.arange(0, 0.02, 0.0025), np.arange(0.02, 0.6, 0.01)])
    PITCH_GRID = np.arange(0.002, 0.3, 0.005)

    def __init__(self, x):
        n = len(x)
        e = envelope(x, 2.0)
        db = 20 * np.log10(e / (e.max() + 1e-12) + 1e-9)
        # points after the end of a cut-out hit (the next hit began) are unknown, not silent
        self.level = np.clip(np.array([db[int(t * SR)] if int(t * SR) < n else np.nan
                                       for t in self.LEVEL_GRID]), -60, 0)
        early = x[:int(0.04 * SR)]
        late = x[int(0.04 * SR):int(0.25 * SR)]
        self.spectrum_early = third_octave(early)
        self.spectrum_late = third_octave(late) if len(late) > int(0.03 * SR) else np.full_like(self.spectrum_early, np.nan)
        self.tonality = tonality(x[:int(0.2 * SR)]) if n > 512 else 0.0

        t, f = pitch_track(x)
        self.pitch = np.full(len(self.PITCH_GRID), np.nan)
        if len(t):
            for k, tk in enumerate(self.PITCH_GRID):
                j = np.argmin(np.abs(t - tk))
                if abs(t[j] - tk) < 0.01 and db[min(int(tk * SR), n - 1)] > -30 and 15 < f[j] < 3000:
                    self.pitch[k] = np.log2(f[j])

    def distance(self, other, weights=None):
        """weighted mean-square distance; parts: level (per 10 dB), spectra (per 15 dB), pitch (octaves)"""
        w = dict(level=1.0, spectrum=1.0, pitch=4.0)
        w.update(weights or {})
        level = np.nanmean(((self.level - other.level) / 10) ** 2)
        late = ((self.spectrum_late - other.spectrum_late) / 15) ** 2
        early = np.mean(((self.spectrum_early - other.spectrum_early) / 15) ** 2)
        spectrum = 0.5 * (early + np.nanmean(late)) if np.any(~np.isnan(late)) else early
        both = ~np.isnan(self.pitch) & ~np.isnan(other.pitch)
        tonal = min(self.tonality, other.tonality)
        pitch = (np.mean((self.pitch[both] - other.pitch[both]) ** 2) if both.sum() > 3 else 0.0) * tonal
        return w['level'] * level + w['spectrum'] * spectrum + w['pitch'] * pitch, dict(level=level, spectrum=spectrum, pitch=pitch)


class Renderer:
    """Keeps PS8Tests --render-stdin running and renders single hits with the given options."""

    def __init__(self, exe):
        self.proc = subprocess.Popen([exe, '--render-stdin'], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                     text=True, bufsize=1)
        self.tmp = tempfile.mkdtemp(prefix='ps8match')
        self.count = 0

    def render(self, options, seconds=1.0):
        self.count += 1
        out = os.path.join(self.tmp, f'r{self.count % 4}.wav')
        line = '|'.join([out, 'hit', f'seconds={seconds}'] + list(options))
        self.proc.stdin.write(line + '\n')
        self.proc.stdin.flush()
        if self.proc.stdout.readline().strip() != 'ok':
            raise RuntimeError('render failed: ' + line)
        return load(out)

    def close(self):
        self.proc.stdin.close()
        self.proc.wait()
