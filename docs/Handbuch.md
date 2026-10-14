# PS-8 – Handbuch

*English: [Manual](Manual.md)*

![PS-8 Frontplatte](images/panel.png)

PS-8 ist ein analog klingender Percussion-Synthesizer mit eingebautem Step-Sequencer. Er hat keine
fertigen Drum-Samples: Jeder Klang – Kick, Tom, Snare-artiges Rauschen, Metall, Zaps, Bassläufe –
entsteht aus zwei Oszillatoren, Rauschen, einem Filter und drei Hüllkurven. Der Sequencer spielt dazu
pro Step eine eigene Tonhöhe und Lautstärke.

Die Kurzfassung steht im [Schnellstart](Schnellstart.md). Dieses Handbuch erklärt jedes Bedienelement,
gibt Klangrezepte und listet alle Parameter.

---

## Inhalt

1. [Installation](#1-installation)
2. [Erste Schritte](#2-erste-schritte)
3. [So entsteht der Klang](#3-so-entsteht-der-klang)
4. [Die Frontplatte](#4-die-frontplatte)
5. [Sequencer](#5-sequencer)
6. [Sequence, LFO und Status](#6-sequence-lfo-und-status)
7. [MIDI](#7-midi)
8. [Klangrezepte](#8-klangrezepte)
9. [Tipps und Tricks](#9-tipps-und-tricks)
10. [Parameterliste](#10-parameterliste)
11. [Technische Daten](#11-technische-daten)
12. [Probleme lösen](#12-probleme-lösen)

---

## 1. Installation

PS-8 gibt es als **AU** (nur macOS), **VST3** und **CLAP** für macOS, Windows und Linux.
Fertige Pakete liegen unter **Releases** im Repository.

| System | Ordner |
|---|---|
| macOS AU | `~/Library/Audio/Plug-Ins/Components/` |
| macOS VST3 | `~/Library/Audio/Plug-Ins/VST3/` |
| macOS CLAP | `~/Library/Audio/Plug-Ins/CLAP/` |
| Windows VST3 | `C:\Program Files\Common Files\VST3\` |
| Windows CLAP | `C:\Program Files\Common Files\CLAP\` |
| Linux VST3 / CLAP | `~/.vst3/` / `~/.clap/` |

Danach in der DAW die Plugins neu scannen lassen. PS-8 erscheint unter dem Hersteller
**sscheppert-blip** als Instrument.

### Plugins auf macOS freigeben

macOS versieht jede heruntergeladene Datei mit einer unsichtbaren „Quarantäne“-Markierung. Bei
Plugins, die nicht von Apple notarisiert sind – wie PS-8 und viele kostenlose oder Open-Source-Plugins –
kann das bedeuten:

- eine Meldung, das Plugin „kann nicht geöffnet werden“ oder „ist beschädigt“,
- oder die DAW überspringt es beim Scannen einfach.

**Der allgemeine Weg – klappt mit jedem Plugin:** die Markierung einmal im **Terminal** entfernen
(Programme → Dienstprogramme → Terminal) und danach die DAW neu starten:

```bash
xattr -dr com.apple.quarantine "/Pfad/zum/Plugin"
```

Tipp: `xattr -dr com.apple.quarantine ` eintippen (mit Leerzeichen am Ende), die Plugin-Datei aus dem
Finder ins Terminal-Fenster ziehen – der Pfad wird eingefügt – und Return drücken. `-d` löscht die
Markierung, `-r` nimmt alles im Plugin-Paket mit. `xattr -l "/Pfad/zum/Plugin"` zeigt, ob noch eine
Markierung da ist.

**Für PS-8 – alle drei Formate auf einmal:**

```bash
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/PS-8.component ~/Library/Audio/Plug-Ins/VST3/PS-8.vst3 ~/Library/Audio/Plug-Ins/CLAP/PS-8.clap
```

Wer nur einzelne Formate installiert hat, bekommt für die fehlenden eine Meldung – das ist egal, die
anderen werden trotzdem freigegeben.

Nur bei Plugins aus Quellen machen, denen du vertraust: Die Markierung ist macOS' Art zu fragen
„Bist du sicher?“.

---

## 2. Erste Schritte

1. PS-8 auf eine **MIDI-/Instrumentenspur** laden.
2. In der DAW **Play** drücken. PS-8 läuft im Takt mit – ab Werk stehen **RUN** und **HOST** an.
3. Es spielt ein gleichmäßiges Pattern aus 8 Steps. Jetzt an den Reglern drehen:
   - **OSCILLATOR 1 → FREQUENCY**: Grundton
   - **OSCILLATOR 1 → EG AMOUNT**: wie stark der Ton bei jedem Schlag herunterfällt („Bwumm“)
   - **FILTER → CUTOFF**: hell / dumpf
   - **DECAY → VCA**: kurz / lang
4. Mit den **PITCH**-Reglern im Sequencer bekommt jeder Step seine eigene Tonhöhe, mit **VELOCITY**
   seine Lautstärke.

**Bedienung allgemein**

| Aktion | Wirkung |
|---|---|
| Regler nach oben/unten ziehen | Wert ändern (beim Ziehen zeigt eine kleine Anzeige den Wert) |
| **Doppelklick** oder **Alt + Klick** auf einen Regler | Zurück auf den Startwert |
| Klick auf einen Kippschalter | Umschalten (beim dreistufigen Schalter: obere Hälfte = nach oben, untere = nach unten) |
| Fensterecke ziehen | Fenster größer/kleiner (Seitenverhältnis bleibt) |

---

## 3. So entsteht der Klang

```
 VCO 1 ──┬──────────────┐
         │ FM           ├─► MIXER ─► FILTER ─► DRIVE ─► VCA ─► OUTPUT
 VCO 2 ◄─┘ (+ SYNC)     │            ▲                  ▲
 NOISE ─────────────────┘            │                  │
                                     │                  │
 PITCH-DECAY ─► Tonhöhe VCO 1/2      │                  │
 FILTER-DECAY ───────────────────────┘                  │
 VCA-DECAY ─────────────────────────────────────────────┘
 SEQUENCER ─► Auslösen + Tonhöhe + Lautstärke pro Step
 LFO ─► Pitch, VCO 2, Cutoff, FM oder Lautstärke
```

Jeder Step des Sequencers (oder jede passende MIDI-Note) **löst alle drei Hüllkurven gleichzeitig aus**.
Die Hüllkurven springen sofort auf voll und klingen dann ab – wie lange, bestimmen die drei
**DECAY**-Regler. Wie stark sie wirken, bestimmen die **EG AMOUNT**-Regler.

---

## 4. Die Frontplatte

### Obere Leiste

| Element | Funktion |
|---|---|
| **OUTPUT** (Zeiger) | Ausgangspegel: links −20 dB, Mitte 0 dB, rechts +3 dB |
| **LOAD** (Zeiger) | Rechenlast des Plugins in Prozent |
| **TEMPO BPM** (Nixies) | Aktuelles Tempo – das eigene oder das der DAW |
| **RUN** (grüne Lampe) | Leuchtet, wenn der Sequencer läuft |
| **STEP** (Nixies) | Zuletzt gespielter Step |
| **SCRAM** | Not-Aus: Sequencer stoppt (RUN geht aus), der Klang verstummt sofort, die Step-Lampen blinken kurz rot |

### OSCILLATOR 1 und OSCILLATOR 2

| Element | Funktion |
|---|---|
| **FREQUENCY** | Grundton, ca. 8 Hz – 8 kHz (von tiefem Brummen bis zu hohem Pfeifen) |
| **EG AMOUNT** | Pitch-Hüllkurve. **Mitte = aus**. Rechts: Ton startet höher und fällt (Kick, Tom). Links: Ton startet tiefer und steigt (Zaps). Bis ±5 Oktaven |
| **WAVE** SQR / TRI | Rechteck (hohl, kräftig) oder Dreieck (weich, rund) |
| **LEVEL** | Lautstärke im Mixer |

Wie schnell die Pitch-Hüllkurve abklingt, stellt **DECAY → PITCH** ein. Sie wirkt auf beide
Oszillatoren, jeder mit seinem eigenen EG AMOUNT.

### MODULATION

| Element | Funktion |
|---|---|
| **FM 1>2 AMOUNT** | VCO 1 moduliert die Frequenz von VCO 2. Wenig: rauer, mehr Obertöne. Viel: metallisch, glockig, Krach |
| **FM** ON / OFF | FM schnell an- und ausschalten, ohne den Regler zu verstellen |
| **HARD SYNC** ON / OFF | VCO 2 startet bei jeder Schwingung von VCO 1 neu. Klingt mit der Pitch-Hüllkurve auf VCO 2 schneidend und „sprechend“ |

### NOISE

| Element | Funktion |
|---|---|
| **LEVEL** | Rauschanteil im Mixer (Snare, Hi-Hat, Anschlag-Klick) |
| **COLOR** WHITE / PINK | Weißes Rauschen (zischend) oder rosa Rauschen (dunkler, rauschender) |
| **VCF MOD** | Rauschen moduliert die Filterfrequenz – macht den Klang körnig, schmutzig |

### FILTER

| Element | Funktion |
|---|---|
| **CUTOFF** | Filterfrequenz, ca. 20 Hz – 20 kHz |
| **RESONANCE** | Betont den Bereich um die Filterfrequenz. Ganz rechts pfeift das Filter selbst (auch ohne Oszillatoren) |
| **EG AMOUNT** | Filter-Hüllkurve. Mitte = aus, rechts öffnet sich das Filter bei jedem Schlag, links schließt es sich. Bis ±8 Oktaven |
| **MODE** LOW / HIGH | Tiefpass (Bass bleibt, Höhen weg) oder Hochpass (Höhen bleiben, Bass weg) – beide mit 24 dB/Oktave |

### DECAY

| Element | Funktion |
|---|---|
| **PITCH** | Abklingzeit der Pitch-Hüllkurve |
| **FILTER** | Abklingzeit der Filter-Hüllkurve |
| **VCA** | Abklingzeit der Lautstärke – also die Länge des Klangs |
| **VCA ATTACK** FAST / SLOW | Harter Anschlag oder weicher Einsatz (ca. 12 ms) |
| **DRIVE** | Sättigung/Verzerrung nach dem Filter |

Alle Decays reichen von ca. 3 ms bis 10 s.

### OUTPUT

| Element | Funktion |
|---|---|
| **VOLUME** | Ausgangslautstärke |
| **CLIP** | Leuchtet rot, wenn der Ausgang über 0 dB geht – dann VOLUME zurücknehmen |

### PATCH

Die Buchsen sind in dieser Version **noch ohne Funktion** (Stempel „OFFLINE“). Das Verkabeln kommt in
einer späteren Version.

---

## 5. Sequencer

| Element | Funktion |
|---|---|
| **PITCH** (obere Reihe) | Tonhöhe je Step, ±24 Halbtöne. Mitte = 0 |
| **VELOCITY** (mittlere Reihe) | Lautstärke je Step |
| **Leuchttaster** (STEP ON) | Step an/aus. **Grün** = an, **dunkel** = aus, **amber** = spielt gerade. Steps hinter der eingestellten Länge sind abgedunkelt |
| **TEMPO** | Eigenes Tempo 20–300 BPM (nur bei **INT**) |
| **RUN / STOP** | Sequencer an/aus |
| **INT / HOST** | **INT**: eigenes Tempo, startet sofort mit RUN. **HOST**: läuft nur, wenn die DAW spielt, und hängt fest am Takt |
| **PITCH TO** 1+2 / OFF / VCO 2 | Wohin die Step-Tonhöhe geht: beide Oszillatoren, nirgends, oder nur VCO 2 |
| **TRIG** | Aktuellen Step nochmal spielen |
| **ADV** | Einen Step weiter und spielen |
| **RESET** | Der nächste Step ist wieder Step 1 |

**HOST-Modus im Detail:** Die Steps hängen an der Position im Song, gezählt ab Songanfang. Bei
8 Steps à 1/16 fällt Step 1 also auf jede halbe Note, bei 16 Steps auf jeden Taktanfang. Springst du in der DAW oder läuft ein Loop,
springt der Sequencer mit. **RESET** verschiebt den Start auf den nächsten Step.

**Velocity und Lautstärke:** Die Velocity eines Steps ist die Spitze der VCA-Hüllkurve. Velocity 0 ist
stumm – genau wie ein ausgeschalteter Step.

---

## 6. Sequence, LFO und Status

Diese Reihe enthält Extras. Ab Werk ist alles neutral: 8 Steps, keine Skala, kein Glide, LFO-Stärke 0.
Unter jedem Regler zeigt eine kleine Anzeige den aktuellen Wert.

### SEQUENCE

| Element | Funktion |
|---|---|
| **RATE** | Notenwert pro Step: 1/4, 1/8, 1/8T, 1/16, 1/16T, 1/32 (T = Triole) |
| **LENGTH** | Anzahl der Steps, 1–16. Damit gehen auch ungerade Rhythmen (5, 7, 11 …), die gegen den Takt laufen |
| **SCALE** | Die PITCH-Regler rasten ein: **OFF** (stufenlos), **CHROMATIC** (Halbtöne), **MAJOR** (Dur), **MINOR** (Moll), **PENTATONIC** (Moll-Pentatonik). Grundton ist immer die eingestellte FREQUENCY |
| **GLIDE** | Der Ton gleitet von Step zu Step statt zu springen, bis 1 s. Ganz links = aus |

### LFO

| Element | Funktion |
|---|---|
| **RATE** | Geschwindigkeit, 0,05–40 Hz. Bei **SYNC** wählt derselbe Regler einen Notenwert: 4 Takte, 2 Takte, 1 Takt, 1/2, 1/4, 1/8, 1/16, 1/32 |
| **SHAPE** | Dreieck, Rechteck, Sägezahn fallend, Sägezahn steigend, Sample & Hold (Zufallsstufen) |
| **DEPTH** | Stärke. 0 = aus |
| **TARGET** | **PITCH** (beide VCOs, bis ±2 Oktaven), **VCO 2** (nur VCO 2), **CUTOFF** (bis ±4 Oktaven), **FM** (FM-Stärke), **VOLUME** (Tremolo) |
| **FREE / SYNC** | Frei laufend oder im Takt der DAW (bei HOST und laufender DAW hängt der LFO fest am Takt) |

### STATUS

| Element | Funktion |
|---|---|
| **STEPS** 1-8 / 9-16 | Welche acht Steps unten im Sequencer angezeigt werden |
| **HOST LOCK** | Leuchtet, wenn PS-8 am Takt der DAW hängt |
| **MIDI IN** | Blinkt bei jeder eingehenden Note |
| **LFO** | Blinkt im Takt des LFO (wenn DEPTH > 0) |
| **STEPS 9-16** | Leuchtet, solange der Sequencer auf Steps 9–16 spielt – auch wenn du gerade 1–8 ansiehst |

---

## 7. MIDI

PS-8 hört auf allen MIDI-Kanälen.

| Note | Funktion |
|---|---|
| **C1 … D#2** (36–51) | Step 1 … 16 direkt spielen. Lautstärke = Notenvelocity × Step-Velocity |
| **E2** (52) | Nächster Step |
| **F2** (53) | Aktuellen Step nochmal |
| **F#2** (54) | Zurück auf Step 1 |

Die Noten funktionieren auch, wenn der Sequencer läuft – dann spielst du zusätzlich zum Pattern.

**Eigene Rhythmen per MIDI:** **RUN** aus, dann die Steps mit einem MIDI-Clip spielen. Jeder Step
ist dann wie ein Drum-Pad mit eigener Tonhöhe – so baust du Patterns beliebiger Länge.

**Automation:** Alle Regler und Schalter sind in der DAW automatisierbar (Namen siehe Parameterliste).

---

## 8. Klangrezepte

Ausgangspunkt jeweils: Plugin frisch laden. Uhrzeiten beziehen sich auf die Reglerstellung
(7 Uhr = ganz links, 12 Uhr = Mitte, 5 Uhr = ganz rechts).

**Tiefe Kick**
- VCO 1: FREQUENCY 9 Uhr, EG AMOUNT 2 Uhr, WAVE TRI, LEVEL voll
- VCO 2 LEVEL und NOISE LEVEL ganz links
- FILTER: CUTOFF 1 Uhr, RESONANCE 9 Uhr, EG AMOUNT Mitte
- DECAY: PITCH 9 Uhr, VCA 12 Uhr

**Tom-Melodie**
- Wie die Kick, aber VCO 1 FREQUENCY 11 Uhr, EG AMOUNT 1 Uhr
- SCALE auf MINOR, PITCH-Regler der Steps verschieden einstellen

**Snare-artig**
- VCO 1: FREQUENCY 11 Uhr, EG AMOUNT 1 Uhr, LEVEL halb
- NOISE: LEVEL 2 Uhr, COLOR WHITE
- FILTER: MODE HIGH, CUTOFF 11 Uhr
- DECAY: PITCH 8 Uhr, VCA 10 Uhr

**Hi-Hat / Metall**
- VCO 1 und VCO 2 auf SQR, FREQUENCY beide 3 Uhr (leicht unterschiedlich)
- FM 1>2 AMOUNT 2 Uhr, NOISE LEVEL 12 Uhr
- FILTER: MODE HIGH, CUTOFF 2 Uhr
- DECAY: VCA 8 Uhr (kurz) – für offene Hats 11 Uhr

**Laser-Zap**
- VCO 1: FREQUENCY 1 Uhr, EG AMOUNT ganz rechts
- DECAY: PITCH 10 Uhr, VCA 10 Uhr
- RESONANCE 2 Uhr

**Acid-Lauf**
- VCO 1 auf SQR, FREQUENCY 10 Uhr, EG AMOUNT Mitte
- FILTER: CUTOFF 9 Uhr, RESONANCE 3 Uhr, EG AMOUNT 2 Uhr
- DECAY: FILTER 10 Uhr, VCA 1 Uhr
- SCALE MINOR, GLIDE 9 Uhr, PITCH-Regler verteilen, LENGTH 16

**Bewegter Groove**
- Beliebiger Sound, dazu LFO: TARGET CUTOFF, SHAPE TRIANGLE, SYNC, RATE 1 BAR, DEPTH 12 Uhr
- Oder TARGET PITCH, SHAPE S&H, RATE 1/16, DEPTH 9 Uhr für zufällige Tonhöhen

---

## 9. Tipps und Tricks

- **Polyrhythmen:** LENGTH auf 5 oder 7 stellen – das Pattern verschiebt sich gegen den Takt und
  wiederholt sich erst nach vielen Takten.
- **Velocity als Klangfarbe:** Kurze VCA-Decays plus unterschiedliche Velocities ergeben
  Ghost-Notes und Akzente.
- **Filter als Oszillator:** VCO- und Noise-LEVEL auf 0, RESONANCE ganz rechts – das Filter
  pfeift selbst und wird von der Filter-Hüllkurve gespielt.
- **Zwei Klänge in einem:** PITCH TO auf **VCO 2**: VCO 1 bleibt als Kick-Fundament stehen, VCO 2
  spielt die Melodie darüber.
- **Mehrere Instanzen:** Für Kick, Snare und Hats einfach mehrere PS-8 laden – jede Instanz ist eine
  Stimme, so wie bei der Hardware.
- **Panik:** **SCRAM** stoppt alles sofort, wenn die Resonanz eskaliert. Mit **RUN** geht es weiter.

---

## 10. Parameterliste

So heißen die Parameter in der Automation der DAW.

| Name | Bereich | Startwert |
|---|---|---|
| VCO 1 Frequency | 8 Hz – 8,2 kHz | 64 Hz |
| VCO 1 EG Amount | −100 … +100 % | +45 % |
| VCO 1 Wave | Triangle / Square | Triangle |
| VCO 1 Level | 0–100 % | 85 % |
| VCO 2 Frequency | 8 Hz – 8,2 kHz | 181 Hz |
| VCO 2 EG Amount | −100 … +100 % | +30 % |
| VCO 2 Wave | Triangle / Square | Square |
| VCO 2 Level | 0–100 % | 35 % |
| FM 1-2 Amount | 0–100 % | 0 % |
| FM | Off / On | On |
| Hard Sync | Off / On | Off |
| Noise Level | 0–100 % | 10 % |
| Noise Color | White / Pink | White |
| Noise VCF Mod | 0–100 % | 0 % |
| Cutoff | 20 Hz – 20,5 kHz | ca. 905 Hz |
| Resonance | 0–100 % | 30 % |
| VCF EG Amount | −100 … +100 % | +40 % |
| Filter Mode | Low Pass / High Pass | Low Pass |
| Pitch Decay | 3 ms – 10 s | ca. 34 ms |
| VCF Decay | 3 ms – 10 s | ca. 77 ms |
| VCA Decay | 3 ms – 10 s | ca. 173 ms |
| VCA Attack | Fast / Slow | Fast |
| Drive | 0–100 % | 0 % |
| Volume | 0–100 % | 70 % |
| Tempo | 20–300 BPM | 120 BPM |
| Run | Stop / Run | Run |
| Clock | Internal / Host | Host |
| Rate | 1/4 … 1/32 | 1/16 |
| Seq Pitch To | VCO 1+2 / Off / VCO 2 | VCO 1+2 |
| Seq Length | 1–16 | 8 |
| Seq Pitch Scale | Off / Chromatic / Major / Minor / Pentatonic | Off |
| Glide | Off – 1 s | Off |
| LFO Rate | 0,05–40 Hz | ca. 1 Hz |
| LFO Sync Rate | 4 Bars … 1/32 | 1 Bar |
| LFO Sync | Free / Sync | Free |
| LFO Shape | Triangle / Square / Saw / Ramp / Sample & Hold | Triangle |
| LFO Depth | 0–100 % | 0 % |
| LFO Target | Pitch / VCO 2 / Cutoff / FM / Volume | Cutoff |
| Step 1 … 16 Pitch | −24 … +24 Halbtöne | 0 |
| Step 1 … 16 Velocity | 0–100 % | 100 % |
| Step 1 … 16 On | Off / On | On |

Die Anzeige „Steps 1-8 / 9-16“ wird mit dem Projekt gespeichert, ist aber nicht automatisierbar.

---

## 11. Technische Daten

| | |
|---|---|
| Formate | AU (macOS), VST3, CLAP; Standalone zum Testen |
| Systeme | macOS 11+ (Apple Silicon und Intel), Windows x64, Linux x64 |
| Stimmen | 1 pro Instanz |
| Oversampling | 4× bis 48 kHz, 2× bis 96 kHz |
| Latenz | wenige Samples (wird der DAW gemeldet) |
| Ausgang | Mono-Signal auf Stereo-Ausgang |
| Filter | 4-Pol-Ladder, Tief- und Hochpass, selbstoszillierend |
| CPU | unter 1 % eines Kerns (gemessen auf einem aktuellen Mac) |

---

## 12. Probleme lösen

**PS-8 taucht in der DAW nicht auf** – Plugin-Ordner prüfen (siehe Installation) und in der DAW neu
scannen. Auf dem Mac das Plugin freigeben (siehe [Plugins auf macOS freigeben](#plugins-auf-macos-freigeben))
und die DAW neu starten.

**Kein Ton** – Läuft die DAW (bei HOST)? Ist RUN an? Stehen LEVEL-Regler und VOLUME auf? Sind die
Steps grün? Steht VELOCITY über 0? Wurde SCRAM gedrückt (dann RUN wieder einschalten)?

**Sequencer läuft nicht im Takt** – Schalter auf **HOST** stellen. Im Standalone gibt es keinen
DAW-Takt, dort läuft PS-8 immer mit eigenem Tempo.

**Es pfeift** – RESONANCE zurückdrehen. Ganz rechts schwingt das Filter absichtlich selbst.

**CLIP leuchtet** – VOLUME oder DRIVE zurücknehmen.

---

*Ideen und Ohren: ein Mensch. Code, DSP und dieses Handbuch: Claude Code (Claude Opus 5.5,
mittlerer Aufwand). Die meisten Blumen gehören dem Roboter.* 🤖💐
