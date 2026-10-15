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
6. [Unter der Haube: die Service-Platine](#6-unter-der-haube-die-service-platine)
7. [Patchbay](#7-patchbay)
8. [Presets](#8-presets)
9. [MIDI](#9-midi)
10. [Klangrezepte](#10-klangrezepte)
11. [Tipps und Tricks](#11-tipps-und-tricks)
12. [Parameterliste](#12-parameterliste)
13. [Technische Daten](#13-technische-daten)
14. [Probleme lösen](#14-probleme-lösen)

---

## 1. Installation

PS-8 gibt es als **AU** (nur macOS), **VST3** und **CLAP** für macOS, Windows und Linux, dazu als
**Standalone**-App, die ohne DAW läuft – praktisch, um schnell Sounds auszuprobieren.
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

**Standalone-App:** auspacken und von überall starten – auf macOS `PS-8.app` (z. B. in den
Programme-Ordner), auf Windows `PS-8.exe`, auf Linux das Programm `PS-8`. Audio- und MIDI-Geräte
unter **Options → Audio/MIDI Settings** wählen. Ohne DAW läuft PS-8 immer mit eigenem **TEMPO**,
auch auf **HOST**.

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

**Für PS-8 – alle Formate auf einmal:**

```bash
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/PS-8.component ~/Library/Audio/Plug-Ins/VST3/PS-8.vst3 ~/Library/Audio/Plug-Ins/CLAP/PS-8.clap /Applications/PS-8.app
```

Wer nur einzelne Formate installiert hat, bekommt für die fehlenden eine Meldung – das ist egal, die
anderen werden trotzdem freigegeben.

Nur bei Plugins aus Quellen machen, denen du vertraust: Die Markierung ist macOS' Art zu fragen
„Bist du sicher?“.

---

## 2. Erste Schritte

1. PS-8 auf eine **MIDI-/Instrumentenspur** laden.
2. **RUN** nach oben schalten (der Sequencer startet ausgeschaltet) und in der DAW **Play** drücken.
   PS-8 läuft im Takt mit – ab Werk steht **HOST** an.
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
| Fensterecke ziehen | Fenster größer/kleiner (Seitenverhältnis bleibt; PS-8 merkt sich die Größe) |

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
 PATCHBAY ─► Kabel leiten all das um (Kapitel 7)
```

Jeder Step des Sequencers (oder jede passende MIDI-Note) **löst alle drei Hüllkurven gleichzeitig aus**.
Die Hüllkurven springen sofort auf voll und klingen dann ab – wie lange, bestimmen die drei
**DECAY**-Regler. Wie stark sie wirken, bestimmen die **EG AMOUNT**-Regler.

---

## 4. Die Frontplatte

### Obere Leiste

| Element | Funktion |
|---|---|
| **PROGRAM** | Preset-Name, ◀ ▶ zum Blättern, SAVE, MENU – siehe [8. Presets](#8-presets) |
| **OUTPUT** (Zeiger) | Ausgangspegel: links −20 dB, Mitte 0 dB, rechts +3 dB |
| **LOAD** (Zeiger) | Rechenlast des Plugins in Prozent |
| **TEMPO BPM** (Nixies) | Aktuelles Tempo – das eigene oder das der DAW |
| **RUN** (grüne Lampe) | Leuchtet, wenn der Sequencer läuft |
| **STEP** (Nixies) | Zuletzt gespielter Step |
| **SCRAM** | Not-Aus: Sequencer stoppt (RUN geht aus), der Klang verstummt sofort, die Step-Lampen blinken kurz rot |
| **SERVICE** (Schlüssel) | Öffnet und schließt die Haube über der Modulreihe – siehe [6. Unter der Haube](#6-unter-der-haube-die-service-platine) |

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

### PATCHBAY

Mit den Buchsen unten leitest du Hüllkurven, Oszillatoren und den Sequencer per Kabel um – siehe
[7. Patchbay](#7-patchbay).

---

## 5. Sequencer

| Element | Funktion |
|---|---|
| **PITCH** (obere Reihe) | Tonhöhe je Step, ±24 Halbtöne. Mitte = 0 |
| **Mittlere Reihe** | Velocity, Ratchet oder Chance je Step – gewählt mit den drei Tasten links neben der Reihe |
| **VELOCITY / RATCHET / CHANCE** (Tasten) | Was die mittlere Reihe einstellt. Die leuchtende Taste ist aktiv; die beiden anderen Werte bleiben, wie sie sind |
| **Leuchttaster** (STEP ON) | Step an/aus. **Grün** = an, **dunkel** = aus, **amber** = spielt gerade. Steps hinter der eingestellten Länge sind abgedunkelt |
| **TEMPO** | Eigenes Tempo 20–300 BPM (nur bei **INT**) |
| **RUN / STOP** | Sequencer an/aus |
| **INT / HOST** | **INT**: eigenes Tempo, startet sofort mit RUN. **HOST**: läuft nur, wenn die DAW spielt, und hängt fest am Takt (mit HOST passiert bei RUN also erst etwas, wenn du in der DAW Play drückst). Die Standalone-App startet auf INT; HOST wartet dort auf einen DAW-Takt, der nie kommt |
| **PITCH TO** 1+2 / OFF / VCO 2 | Wohin die Step-Tonhöhe geht: beide Oszillatoren, nirgends, oder nur VCO 2 |
| **TRIG** | Aktuellen Step nochmal spielen |
| **ADV** | Einen Step weiter und spielen |
| **RESET** | Der nächste Step ist wieder Step 1 |
| **RATE** (Kopfzeile links) | Notenwert pro Step: 1/4, 1/8, 1/8T, 1/16, 1/16T, 1/32 (T = Triole) |
| **LENGTH** (Kopfzeile links) | Anzahl der Steps, 1–16. Damit gehen auch ungerade Rhythmen (5, 7, 11 …), die gegen den Takt laufen |
| **1-8 / 9-16** (Reiter in der Kopfzeile) | Welche acht Steps angezeigt werden. Ein Punkt leuchtet auf der Seite, die gerade spielt |
| **PATTERN 1–8** (Kopfzeile) | Acht Patterns pro Preset; das helle spielt. Amber-Nummern haben Steps drin, blasse sind leer. Rechtsklick auf eine Taste: Pattern in ein anderes kopieren oder leeren |
| **◀◀ ▶▶** (Kopfzeile) | Das ganze Pattern einen Step nach links / rechts schieben (rotiert innerhalb von LENGTH) |
| **COPY / PASTE** | Die acht angezeigten Steps kopieren und auf die angezeigte Seite einfügen – in eine andere Seite oder ein anderes Pattern |
| **RAND** | Zufall für Pitch, Velocity, Ratchet, Chance, Steps an/aus – oder alles – für die Steps innerhalb von LENGTH |
| **UNDO** | Nimmt das letzte Werkzeug zurück (Schieben, Einfügen, RAND, Leeren, Kopieren in dieses Pattern). Nochmal gedrückt, stellt es es wieder her |

**RATE und LENGTH:** Pfeil anklicken schaltet weiter, ein Klick auf den Wert öffnet eine Liste, das
Mausrad geht auch; Doppelklick auf den Wert setzt zurück.

**HOST-Modus im Detail:** Die Steps hängen an der Position im Song, gezählt ab Songanfang. Bei
8 Steps à 1/16 fällt Step 1 also auf jede halbe Note, bei 16 Steps auf jeden Taktanfang. Springst du in der DAW oder läuft ein Loop,
springt der Sequencer mit. **RESET** verschiebt den Start auf den nächsten Step.

**Velocity und Lautstärke:** Die Velocity eines Steps ist die Spitze der VCA-Hüllkurve. Velocity 0 ist
stumm – genau wie ein ausgeschalteter Step. Wie beim Original klingen leisere Steps außerdem etwas
schneller ab: Bei 100 % brauchen die drei Hüllkurven genau die Zeit des DECAY-Reglers, bei 0 % sind sie
30 % kürzer.

**Ratchet:** 1x–4x je Step. Der Step feuert so viele Schläge, gleichmäßig über seine Länge verteilt –
2x bei einem 1/16-Step ergibt zwei 1/32-Schläge. Ein neuer Step schneidet den Rest ab. Mit einem Kabel
in ADV/CLK ist die Step-Länge der Abstand der letzten beiden Clock-Impulse. TRIG, ADV und MIDI-Noten
spielen die Ratchets mit.

**Chance:** 0–100 % je Step. Bei jedem Durchlauf spielt der Step mit dieser Wahrscheinlichkeit – bei
100 % immer (Standard), bei 0 % nie. Gewürfelt wird nur beim Takt (INT, HOST oder Kabel in ADV/CLK);
TRIG, ADV und MIDI-Noten spielen immer.

**Patterns:** Ein Pattern enthält die Steps (Pitch, Velocity, an/aus, Ratchet, Chance) und LENGTH; RATE,
Swing, Scale und der Klang gehören zum Preset. Ein Klick auf eine Pattern-Taste wechselt sofort – der
Sequencer bleibt an seiner Position. Das spielende Pattern liegt immer auf den Reglern, Automation und
Regler funktionieren also wie gewohnt. Alle acht Patterns werden mit dem Projekt und im Preset
gespeichert.

**RAND** nimmt ganze Halbtöne innerhalb einer Oktave nach oben und unten (SCALE und ROOT gelten weiter)
und lässt Step 1 an, damit das Pattern seine Eins behält.

**Warum zwei gleiche Steps verschieden klingen können:** Die Oszillatoren laufen frei durch, wie
analoge – ein Schlag beginnt dort, wo die Welle gerade steht. Bei tiefen Tönen (Kicks, Toms) ändert
das Klick und Druck von Step zu Step. Für exakt gleiche Schläge **TRIG → VCO RST** stecken (siehe
Kapitel 7).

---

## 6. Unter der Haube: die Service-Platine

![PS-8 Service-Platine](images/service.png)

In der Modulreihe ist nur, was zum Original gehört. Die Klang-Extras sitzen auf einer Platine darunter –
wie hinter der Wartungsklappe einer Leitwarte. Alles, was du beim Spielen des Sequencers brauchst
(Rate, Länge, Ratchet, Chance), bleibt vorne.

- **Öffnen:** den **SERVICE**-Schlüssel drehen (oben rechts neben SCRAM) oder auf eine der vier
  Schrauben an den Ecken der Modulreihe klicken. Die Schrauben drehen sich raus, die Haube klappt hoch.
- **Schließen:** den Schlüssel zurückdrehen oder auf eines der vier Schraubenlöcher der Platine klicken.
- Ob die Haube offen ist, wird mit dem Projekt gespeichert. Presets ändern daran nichts.

Ab Werk ist hier alles neutral: keine Skala, kein Glide, kein Swing, LFO-Stärke 0. Die blauen Trimmpotis
bedienst du wie die Regler (ziehen, Doppelklick setzt zurück); darunter zeigt eine kleine Anzeige den Wert.

### SEQ OPTIONS

| Element | Funktion |
|---|---|
| **SCALE** | Die PITCH-Regler rasten ein: **OFF** (stufenlos), **CHROMATIC** (Halbtöne), **MAJOR** (Dur), **MINOR** (Moll), **PENTATONIC** (Moll-Pentatonik) |
| **ROOT** | Transponiert das ganze Pattern, C … B: Ein Step auf 0 spielt den Grundton, C = unverändert, D = 2 Halbtöne höher … B = 11 höher (gezählt ab der eingestellten FREQUENCY). Wirkt bei jeder SCALE-Einstellung – die Skala baut auf dem Grundton auf |
| **GLIDE** | Der Ton gleitet von Step zu Step statt zu springen, bis 1 s. Ganz links = aus |
| **SWING** | 50 % (gerade) … 75 %: jeder zweite Step kommt später – 66 % ist ein Triolen-Shuffle. Wirkt bei INT und HOST (bei HOST passend zum Takt), nicht mit Kabel in ADV/CLK |

### LFO

| Element | Funktion |
|---|---|
| **RATE** | Geschwindigkeit, 0,05–40 Hz. Bei **SYNC** wählt dasselbe Trimmpoti einen Notenwert: 4 Takte, 2 Takte, 1 Takt, 1/2, 1/4, 1/8, 1/16, 1/32 |
| **SHAPE** | Dreieck, Rechteck, Sägezahn fallend, Sägezahn steigend, Sample & Hold (Zufallsstufen) |
| **DEPTH** | Stärke. 0 = aus |
| **TARGET** | **PITCH** (beide VCOs, bis ±2 Oktaven), **VCO 2** (nur VCO 2), **CUTOFF** (bis ±4 Oktaven), **FM** (FM-Stärke), **VOLUME** (Tremolo) |
| **LFO SYNC** (Jumper) | **FREE** oder **SYNC**: Klick auf den Jumper steckt ihn um. Bei SYNC, HOST und laufender DAW hängt der LFO fest am Takt |

### STATUS (LEDs)

| LED | Funktion |
|---|---|
| **HOST LOCK** | Leuchtet, wenn PS-8 am Takt der DAW hängt |
| **MIDI IN** | Blinkt bei jeder eingehenden Note |
| **LFO** | Blinkt im Takt des LFO (wenn DEPTH > 0) |

---

## 7. Patchbay

Mit der Patchbay unten leitest du Signale per Kabel um. Ohne Kabel klingt und funktioniert PS-8 genau
wie vorher: Intern ist alles schon verbunden. Ein Kabel ändert nur den Eingang, in dem es steckt.

| Aktion | Ergebnis |
|---|---|
| Von einer Buchse zu einer anderen ziehen | Kabel stecken (immer Ausgang mit Eingang, Richtung egal) |
| Stecker aus einem Eingang ziehen | Kabel in einen anderen Eingang umstecken – oder irgendwo sonst loslassen, dann ist es weg |
| Doppelklick oder Rechtsklick auf eine Buchse | Alle Kabel an dieser Buchse entfernen |

Ein Ausgang kann mehrere Eingänge versorgen. Mehrere Kabel in einem Eingang werden addiert. Die Kabel
werden mit dem Projekt und mit Presets gespeichert, lassen sich aber nicht automatisieren.

**Pegel:** Hüllkurven und VELOCITY laufen von 0 bis 5 V, Oszillatoren und VCA schwingen ±5 V, TRIG ist
ein 5-V-Impuls von 5 ms. TRIG, RUN, ADV/CLK und RESET schalten bei 2,5 V. Bei Eingängen, die zu einem
Regler addieren, deckt 5 V den ganzen Reglerweg ab.

### Ausgänge (schwarz)

| Buchse | Signal |
|---|---|
| **VCO 1**, **VCO 2** | Die Wellenformen der Oszillatoren |
| **VCO EG** | Pitch-Hüllkurve |
| **VCF EG** | Filter-Hüllkurve |
| **VCA EG** | VCA-Hüllkurve (inklusive Step-Velocity) |
| **TRIG** | Kurzer Impuls bei jedem Sequencer-Step (auch bei MIDI-Noten und den Tasten TRIG/ADV) |
| **VELOCITY** | Velocity des aktuellen Steps |
| **PITCH** | Tonhöhe des aktuellen Steps, 1 V pro Oktave |
| **VCA** | Der fertige Klang nach dem VCA, vor VOLUME |

### Eingänge

| Buchse | Mit Kabel | Ohne Kabel |
|---|---|---|
| **VCO 1 CV**, **VCO 2 CV** | Tonhöhe, 1 V pro Oktave. PITCH in VCO 1 CV verschiebt VCO 1 genau um die Step-Tonhöhe | – |
| **VCO RST** *(Extra)* | Eine steigende Flanke startet beide VCOs an derselben Stelle der Welle neu. TRIG → VCO RST lässt jeden Schlag gleich klingen | VCOs laufen frei |
| **1>2 FM** | Addiert zu FM 1>2 AMOUNT | – |
| **VCO DEC** | Addiert zum PITCH-Decay | – |
| **NOISE** | Addiert zum Noise-LEVEL | – |
| **EXT IN** | Ersetzt das Rauschen im Mixer (Pegel: Noise-LEVEL) | Rauschen, oder das Sidechain-Audio der DAW |
| **VCF MOD** | Moduliert den Cutoff; Noise-VCF-MOD bestimmt die Stärke | Rauschen |
| **VCF DEC** | Addiert zum FILTER-Decay | – |
| **TRIG** | Eine steigende Flanke feuert alle drei Hüllkurven (der Sequencer läuft weiter) | – |
| **VELOCITY** | Addiert zur Velocity jedes Schlags | – |
| **VCA CV** | Öffnet den VCA direkt, statt der VCA-Hüllkurve | VCA EG |
| **VCA DEC** | Addiert zum VCA-Decay | – |
| **TEMPO** | Beschleunigt den Takt: +1 V verdoppelt das Tempo (nur bei **INT**) | – |
| **RUN** | Der Sequencer läuft nur, solange das Signal hoch ist (und RUN an ist) | Läuft immer |
| **ADV/CLK** | Externer Takt: Jede steigende Flanke spielt den nächsten Step, der eigene Takt pausiert. RUN muss an sein | Eigener oder DAW-Takt |
| **RESET** | Steigende Flanke: Der nächste Step ist Step 1 | – |

**Audio aus der DAW:** PS-8 hat einen Sidechain-Eingang. Leite in der DAW über das Sidechain-Routing
eine Spur darauf. Solange er aktiv ist und kein Kabel in EXT IN steckt, nimmt sein Audio den Platz des
Rauschens ein – es läuft wie das Rauschen durch Filter, VCA und Hüllkurven.

### Patch-Ideen

- **Wirbel:** VCA EG → TEMPO. Jeder Schlag beschleunigt kurz den Takt – Rolls und Bursts, die dem
  VCA-Decay folgen.
- **Filter folgt der Melodie:** PITCH → VCF MOD, Noise-VCF-MOD aufdrehen. Höhere Steps klingen heller.
- **Knurren:** VCO 2 → VCF MOD. Das Filter wird mit Audiorate moduliert – rau und metallisch.
- **Drone:** VELOCITY → VCA CV. Der VCA bleibt offen; jeder Step ändert die Tonhöhe, die Velocity
  den Pegel.
- **Akzente formen den Klang:** VELOCITY → VCF DEC. Laute Steps bekommen einen längeren Filter-Sweep.
- **Rückkopplung:** VCA → EXT IN, Noise-LEVEL vorsichtig aufdrehen. Der Klang speist sich selbst – dreckig
  und instabil.
- **Gleiche Schläge:** TRIG → VCO RST. Jede Kick hat denselben Klick und Druck, wie bei einer klassischen
  Drum Machine. (Deep Kick und Tom Melody sind ab Werk so gepatcht.)
- **VCO als Takt:** VCO 1 FREQUENCY ganz links, VCO 1 → ADV/CLK, VCO-1-LEVEL runter. Jetzt taktet VCO 1
  den Sequencer; mit VCO-1-EG-AMOUNT verbiegt jeder Schlag das Tempo.

---

## 8. Presets

Das **PROGRAM**-Display im Typenschild zeigt das aktuelle Preset. Ein `*` hinter dem Namen heißt: Du hast
seit dem Laden etwas verändert. Oben rechts steht, ob es ein **FACTORY**- oder ein **USER**-Preset ist.

| Element | Funktion |
|---|---|
| **◀ ▶** | Voriges / nächstes Preset: Init, dann die Werks-Presets, dann deine eigenen (am Ende geht es von vorn los) |
| **SAVE** | Den aktuellen Klang als eigenes Preset speichern. Gibt es den Namen schon, fragt PS-8 vor dem Überschreiben nach |
| **MENU** oder Klick auf den Namen | Alle Presets auf einen Blick, dazu **Save as…** (Speichern unter), **Rename…** (Umbenennen), **Delete** (Löschen) und **Open preset folder** (Ordner öffnen) |

**Was ein Preset enthält:** alle Regler und Schalter, alle acht Step-Patterns (Pitch, Velocity, an/aus,
Ratchet, Chance, Länge) und welches davon spielt, Rate, Scale, Root, Glide, Swing, den LFO und die
Patch-Kabel. **Nicht** dabei sind **RUN/STOP** und
**INT/HOST** – ein Preset-Wechsel startet oder stoppt den Sequencer also nie.

**Werks-Presets** sind eingebaut und schreibgeschützt. Willst du eine geänderte Fassung behalten,
drück **SAVE** und gib ihr einen Namen. Mit dabei:

| Preset | |
|---|---|
| Deep Kick, Tom Melody, Snare, Closed Hat, Open Hat, Laser Zap, Acid Line | Die Rezepte aus Kapitel 10 (Kick und Tom mit TRIG → VCO RST für gleichmäßige Schläge) |
| Moving Groove, Random Pitches | LFO auf den Cutoff / zufällige Step-Tonhöhen, im DAW-Takt |
| Rolls (INT) | VCA EG → TEMPO. Den Takt auf **INT** stellen – der TEMPO-Eingang wirkt nur mit dem eigenen Takt |
| Growl, Drone, Feedback, Melodic Filter | Die Patch-Ideen aus Kapitel 7 |

**Wo deine Presets liegen** – eine Datei pro Preset (`.ps8preset`, reines XML). So kannst du sie
sichern, auf einen anderen Rechner kopieren oder weitergeben:

| System | Ordner |
|---|---|
| Windows | `%APPDATA%\PS-8\Presets\` |
| macOS | `~/Library/Application Support/PS-8/Presets/` |
| Linux | `~/.config/PS-8/Presets/` |

Preset-Dateien, die du in diesen Ordner legst, tauchen beim nächsten Öffnen des Menüs auf. Gelöschte
Presets landen im Papierkorb.

**Projekte und neue Instanzen:** Ein neues PS-8 startet immer mit **Init** – auch die Standalone-App.
Ein gespeichertes Projekt bringt genau den Klang zurück, mit dem es gespeichert wurde, inklusive
Preset-Name. Außerdem merkt sich PS-8 die **Fenstergröße** für alle Instanzen (`settings.xml` neben dem
Ordner `Presets`).

---

## 9. MIDI

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

## 10. Klangrezepte

Ausgangspunkt jeweils: Plugin frisch laden (oder **Init** im PROGRAM-Menü wählen). Die meisten
Rezepte sind auch als Werks-Presets eingebaut. Uhrzeiten beziehen sich auf die Reglerstellung
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
- LENGTH 16; Service-Platine: SCALE MINOR, GLIDE 9 Uhr; dann die PITCH-Regler verteilen

**Bewegter Groove**
- Beliebiger Sound, dazu der LFO auf der Service-Platine: TARGET CUTOFF, SHAPE TRIANGLE, Jumper auf SYNC, RATE 1 BAR, DEPTH 12 Uhr
- Oder TARGET PITCH, SHAPE S&H, RATE 1/16, DEPTH 9 Uhr für zufällige Tonhöhen

---

## 11. Tipps und Tricks

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

## 12. Parameterliste

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
| Run | Stop / Run | Stop |
| Clock | Internal / Host | Host |
| Rate | 1/4 … 1/32 | 1/16 |
| Seq Pitch To | VCO 1+2 / Off / VCO 2 | VCO 1+2 |
| Seq Length | 1–16 | 8 |
| Seq Pitch Scale | Off / Chromatic / Major / Minor / Pentatonic | Off |
| Seq Root | C … B | C |
| Swing | 50–75 % | 50 % |
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
| Step 1 … 16 Ratchet | 1x–4x | 1x |
| Step 1 … 16 Chance | 0–100 % | 100 % |

Die Anzeige „Steps 1-8 / 9-16“, die Wahl VELOCITY / RATCHET / CHANCE, die offene oder geschlossene Haube und die Patch-Kabel werden mit dem Projekt gespeichert, sind aber nicht automatisierbar.

---

## 13. Technische Daten

| | |
|---|---|
| Formate | AU (macOS), VST3, CLAP, Standalone-App |
| Systeme | macOS 11+ (Apple Silicon und Intel), Windows x64, Linux x64 |
| Stimmen | 1 pro Instanz |
| Oversampling | 4× bis 48 kHz, 2× bis 96 kHz |
| Latenz | wenige Samples (wird der DAW gemeldet) |
| Ausgang | Mono-Signal auf Stereo-Ausgang |
| Eingang | optionaler Sidechain (mono oder stereo) für EXT IN |
| Patchbay | 17 Eingänge, 9 Ausgänge |
| Filter | 4-Pol-Ladder, Tief- und Hochpass, selbstoszillierend |
| CPU | unter 1 % eines Kerns (gemessen auf einem aktuellen Mac) |

---

## 14. Probleme lösen

**PS-8 taucht in der DAW nicht auf** – Plugin-Ordner prüfen (siehe Installation) und in der DAW neu
scannen. Auf dem Mac das Plugin freigeben (siehe [Plugins auf macOS freigeben](#plugins-auf-macos-freigeben))
und die DAW neu starten.

**Kein Ton** – Läuft die DAW (bei HOST)? Ist RUN an? Stehen LEVEL-Regler und VOLUME auf? Sind die
Steps grün? Steht VELOCITY über 0? Wurde SCRAM gedrückt (dann RUN wieder einschalten)? Bremst ein
Kabel in RUN, ADV/CLK oder VCA CV (Doppelklick auf die Buchse zieht es ab)?

**Sequencer startet nicht** – Mit **HOST** läuft er nur, solange die DAW spielt: in der DAW Play drücken
oder auf **INT** stellen. In der Standalone-App **INT** nehmen (dort ab Werk eingestellt).

**Sequencer läuft nicht im Takt** – Schalter auf **HOST** stellen.

**Es pfeift** – RESONANCE zurückdrehen. Ganz rechts schwingt das Filter absichtlich selbst.

**CLIP leuchtet** – VOLUME oder DRIVE zurücknehmen.

---

*Ideen und Ohren: ein Mensch. Code, DSP und dieses Handbuch: Claude Code (Claude Opus 5.5,
mittlerer Aufwand). Die meisten Blumen gehören dem Roboter.* 🤖💐
