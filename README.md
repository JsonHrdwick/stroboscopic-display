# Vibratory Stroboscopic Display — ESP32 + Electromagnet

## Concept

A 12V electromagnet drives a flexible reed (hacksaw blade, ruler, spring steel strip) at
its resonant frequency. A feather, ribbon, or other light subject is clipped to the free tip.
A strobe LED fires in sync with the drive signal, making the vibrating subject appear frozen
or moving in slow motion.

The ESP32 generates both signals from the same timebase, so the phase relationship between
drive and strobe is exact and tunable. Adjusting the phase "rotates" the frozen image through
the vibration cycle. Adjusting the strobe frequency slightly off the drive frequency creates
a slow-motion playback effect.

```
                    ┌──────────────────────────────────────────┐
                    │          VIBRATORY STROBE DISPLAY         │
                    │                                           │
                    │   ┌───────────────────────────────────┐  │
                    │   │  Feather / Ribbon / Subject       │  │
                    │   └──────────────┬────────────────────┘  │
                    │                  │ attached to tip        │
                    │   ┌──────────────▼────────────────────┐  │
                    │   │  Spring steel reed / hacksaw blade │  │
                    │   └──────────────┬────────────────────┘  │
                    │         clamp ───┤                        │
                    │                  │ EM pulls free end      │
                    │             ┌────▼────┐                   │
                    │             │   12V   │  ← MOSFET Q1      │
                    │             │   E.M.  │                   │
                    │             └─────────┘                   │
                    │                                           │
                    │   ┌─────────────────────────────────┐     │
                    │   │  ESP32   freq ──► EM (Q1)       │     │
                    │   │          freq + phase ──► LED   │     │
                    │   └─────────────────────────────────┘     │
                    │                                           │
                    │   ┌─────────────┐                         │
                    │   │  Strobe LED │ ← MOSFET Q2             │
                    │   └─────────────┘                         │
                    └──────────────────────────────────────────┘
```

## Key Parameters

| Parameter         | Range / Value          |
|-------------------|------------------------|
| Drive frequency   | 5–120 Hz (tunable)     |
| Strobe frequency  | 5–120 Hz (independent) |
| Phase offset      | 0–360° (tunable)       |
| LED pulse width   | 0.5–5 ms               |
| EM supply         | 12V                    |
| MCU               | ESP32                  |

## Interesting Operating Modes

| EM freq | Strobe freq | Effect                                    |
|---------|-------------|-------------------------------------------|
| F       | F           | Subject appears completely frozen         |
| F       | F × 0.99    | Subject appears to vibrate in slow motion |
| F       | F × 0.5     | Shows subject at 2 positions alternately  |
| F       | F × 2       | Shows two frozen phases per cycle         |
| F sweep | F           | Find resonant frequency (max amplitude)   |

## Project Structure

```
stroboscopic-display/
├── README.md
├── design/
│   ├── system-design.md        ← signal flow, timing, operating modes
│   ├── hardware-design.md      ← circuit, BOM, component notes
│   ├── mechanical-design.md    ← reed, mounting, tuning
│   └── firmware-design.md      ← ESP32 code architecture
├── schematics/
│   └── main-schematic.md       ← ASCII schematic
└── src/                        ← PlatformIO firmware (next phase)
```
