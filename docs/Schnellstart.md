# PS-8 – Schnellstart

Ausführlich: [Handbuch](Handbuch.md).

PS-8 ist ein Percussion-Synth mit eingebautem Step-Sequencer (bis 16 Steps, 8 Patterns pro Preset). Statt fester Drum-Sounds baust du
jeden Klang selbst aus zwei Oszillatoren, Rauschen, Filter und drei Decay-Hüllkurven – und der
Sequencer spielt dazu pro Step eine eigene Tonhöhe und Lautstärke.

---

## Loslegen in der DAW

1. PS-8 auf eine MIDI-Spur laden.
2. **RUN** nach oben schalten (ab Werk ist der Sequencer aus) und den Transport starten – der
   Sequencer läuft im Takt mit (Schalter **HOST**).
3. An **FREQUENCY**, **EG AMOUNT**, **CUTOFF** und den **DECAY**-Reglern drehen.

Doppelklick auf einen Regler setzt ihn zurück. Beim Drehen zeigt eine kleine Anzeige den Wert.

## Die Bereiche

| Bereich | Was er macht |
|---|---|
| **OSCILLATOR 1 / 2** | Tonhöhe, Pitch-Hüllkurve (Mitte = aus, rechts = Ton fällt wie bei einer Kick, links = Ton steigt), Wellenform, Lautstärke |
| **MODULATION** | **FM**: VCO 1 moduliert VCO 2 (metallisch, glockig). **HARD SYNC**: VCO 2 startet mit VCO 1 neu. |
| **NOISE** | Rauschen (weiß/rosa) und **VCF MOD**: Rauschen wackelt am Filter |
| **FILTER** | Cutoff, Resonanz (ganz rechts pfeift es selbst), Hüllkurven-Anteil, Tief-/Hochpass |
| **DECAY** | Abklingzeit für Pitch, Filter und Lautstärke. **VCA ATTACK SLOW** macht den Anschlag weicher. **DRIVE** zerrt |
| **OUTPUT** | Lautstärke, **CLIP**-Lampe bei Übersteuerung |

## Sequencer

| Element | Funktion |
|---|---|
| **PITCH** (obere Reihe) | Tonhöhe je Step, ±2 Oktaven (mit **SCALE** eingerastet) |
| **Mittlere Reihe** | Je nach Taste links daneben: **VELOCITY** (Lautstärke), **RATCHET** (1–4 Schläge pro Step) oder **CHANCE** (Wahrscheinlichkeit) |
| **Leuchttaster** | Step an/aus. Amber = läuft gerade |
| **TEMPO** | Eigenes Tempo (nur bei **INT**) |
| **RUN / STOP** | Sequencer an/aus |
| **INT / HOST** | Eigenes Tempo oder Takt der DAW (dann laufen die Steps nur, solange die DAW spielt). Standalone-App: ab Werk INT |
| **PITCH TO** | Step-Pitch geht auf beide VCOs, aus, oder nur auf VCO 2 |
| **TRIG / ADV / RESET** | Aktuellen Step nochmal / nächster Step / zurück auf Step 1 |
| **SCRAM** | Not-Aus: Sequencer stoppt, Sound ist sofort weg |
| **RATE / LENGTH** | Links in der Sequencer-Kopfzeile: Notenwert pro Step (1/4 … 1/32) und Anzahl Steps (1–16). Pfeile, Klick auf den Wert für eine Liste, oder Mausrad |
| **1-8 / 9-16** | Reiter in der Sequencer-Kopfzeile: welche Steps angezeigt werden (Punkt = spielt gerade). Ab LENGTH 9 gibt es die zweite Seite |
| **PATTERN 1–8** | Acht Patterns pro Preset, Klick wechselt sofort. Rechtsklick: kopieren / leeren |
| **◀◀ ▶▶ · COPY · PASTE · RAND · UNDO** | Pattern verschieben, Seite kopieren und einfügen, Zufall (Menü), letzten Schritt zurücknehmen |

## Unter der Haube (Extras – ab Werk aus)

Schlüssel **SERVICE** drehen (oben rechts) oder auf eine Schraube an den Ecken der Modulreihe klicken:
die Haube klappt hoch, darunter liegt die Service-Platine mit blauen Trimmpotis. Zuklappen genauso.

| Element | Funktion |
|---|---|
| **SCALE** | Step-Pitch rastet ein: OFF (stufenlos), Halbtöne, Dur, Moll, Pentatonik |
| **ROOT** | Grundton: transponiert das Pattern (C = unverändert, D = 2 Halbtöne höher …); die Skala baut darauf auf |
| **GLIDE** | Ton gleitet von Step zu Step statt zu springen |
| **SWING** | Jeder zweite Step kommt später: 50 % gerade, 66 % Shuffle, 75 % maximal |
| **LFO RATE** | Geschwindigkeit. Mit Jumper auf **SYNC** in Notenwerten (4 Takte … 1/32) im DAW-Takt |
| **LFO SHAPE** | Dreieck, Rechteck, Sägezahn ab/auf, Sample & Hold (Zufall) |
| **LFO DEPTH** | Stärke (0 = aus) |
| **LFO TARGET** | Ziel: Pitch beider VCOs, nur VCO 2, Cutoff, FM-Stärke, Lautstärke |
| **ANALOG** | 0 % = ideale Schaltung. Aufgedreht: leicht verstimmte VCOs, Drift, jeder Schlag minimal anders – jede Instanz ist ein eigenes „Gerät“ |
| **LEDs** | HOST LOCK = läuft im DAW-Takt, MIDI IN = Note kommt an, LFO blinkt mit |

Leuchttaster hinter der eingestellten Länge sind abgedunkelt.

## Per MIDI spielen

| Note | Funktion |
|---|---|
| C1 … D#2 | Step 1 … 16 direkt spielen (Velocity der Note × Step-Velocity) |
| E2 | Nächster Step |
| F2 | Aktuellen Step nochmal |
| F#2 | Zurück auf Step 1 |

Tipp: **RUN** aus und die Steps per MIDI-Clip spielen – so baust du beliebig lange Rhythmen.

## Presets

Oben im Typenschild: **◀ ▶** blättert durch die Presets (Init, Werks-Presets, deine eigenen),
**SAVE** speichert den aktuellen Klang samt Kabeln, **MENU** (oder Klick auf den Namen) zeigt alle und
kann umbenennen, löschen und den Ordner öffnen. Ein `*` hinter dem Namen = geändert. Deine Presets liegen
unter `%APPDATA%\PS-8\Presets` (Windows), `~/Library/Application Support/PS-8/Presets` (Mac) bzw.
`~/.config/PS-8/Presets` (Linux). Neue Instanzen starten immer mit Init; die Fenstergröße merkt sich
PS-8. Mehr: [Handbuch, Kapitel 8](Handbuch.md#8-presets).

## Patchbay

Unten leitest du Signale per Kabel um. Ohne Kabel bleibt alles wie ab Werk.

| Aktion | Ergebnis |
|---|---|
| Von Buchse zu Buchse ziehen | Kabel stecken (Ausgang ↔ Eingang) |
| Stecker aus einem Eingang ziehen | Umstecken – oder ins Leere ziehen, dann ist das Kabel weg |
| Doppelklick / Rechtsklick auf eine Buchse | Alle Kabel dort entfernen |

Ausgänge haben ein schwarzes Feld. Zum Ausprobieren: **VCA EG → TEMPO** (Wirbel), **VCO 2 → VCF MOD**
(Knurren), **VELOCITY → VCA CV** (Drone), **TRIG → VCO RST** (jeder Schlag klingt gleich – die VCOs
laufen sonst frei wie beim analogen Original). Alle Buchsen und mehr Ideen: [Handbuch, Kapitel 7](Handbuch.md#7-patchbay).
