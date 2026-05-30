# Hardware Design

## Schematic Overview

```
12V PSU → [LM317 12V→5.17V linear reg] → ESP32 VIN
       ↘ Q1 MOSFET → Electromagnet
       ↘ Q2 MOSFET → LED strobe
```

No hall sensor. No PID. Circuit is straightforwardly simple.

## Electromagnet Driver — Q1

The EM is a purely resistive+inductive load on a one-way (pull-only) square wave drive.

**MOSFET: IRLZ44N or IRLB8721** (same recommendation as before, either works)

- IRLZ44N: Vds=55V, Id=47A, Rds(on)~35mΩ @ Vgs=3.3V — fine for this application
- IRLB8721: better at Vgs=3.3V (Rds(on)=11mΩ @ Vgs=4.5V) — preferred if you have it

At 50% duty, 12V, ~500mA average current → MOSFET dissipates < 0.1W. No heatsink needed.

Gate circuit (same as before):
```
GPIO5 ──[100Ω]──┬── MOSFET Gate
                │
              [10kΩ]
                │
               GND
```

**Flyback diode is critical.** The electromagnet's inductance will spike to 50–100V when
the MOSFET switches off if there is no flyback path. This destroys the MOSFET.

Use a **1N4001** (or any standard rectifier / Schottky). Reverse-recovery speed is
irrelevant at the 5–120 Hz switching frequency, so a slow rectifier works fine; a
Schottky (e.g. 1N5819) is only preferred at much higher switching rates:
```
12V ──[Cathode]──[1N4001]──[Anode]── MOSFET Drain (EM-)
```

### EM Drive Frequency & Core Saturation

Most 12V hobby electromagnets have an iron core. Iron saturates at ~1.5T, and at 50 Hz
the core losses start increasing. Most cheap electromagnets can be driven at 60–120 Hz
without visible heating, but watch the temperature — if the coil gets hot at your target
frequency, reduce the duty cycle (e.g., 30% instead of 50%).

The electromagnet fires **once per drive cycle** (pull on, release off). This means:
- At 80 Hz drive: 80 pulls/second → reed tip vibrates at 80 Hz
- The reed must be tuned so its resonant frequency matches the drive (see mechanical-design.md)

### Operate above flicker fusion (~75–90 Hz)

This is the central design constraint, and it's why the operating band is set to **60–120 Hz
(default 80 Hz)** rather than something slower. The LED strobes **once per vibration cycle**,
so the flash rate equals the drive frequency. Below the ~60 Hz human flicker-fusion
threshold the strobe is *visibly blinking* — it looks like a cheap disco strobe. At ~80 Hz
the eye fuses the flashes into **continuous light**: the frame appears softly, steadily lit,
and the subject just seems to move in slow motion. That "always on" quality is the whole
effect.

**The slow motion does not come from a low frequency.** It comes from the small offset
between the strobe and the vibration:

```
vibration = 80.0 Hz,  strobe = 79.5 Hz  →  0.5 Hz beat  →  subject loops once every 2 s
```

Both run fast (above fusion); only their *difference* is slow. This mirrors Jeff Lieberman's
*Slow Dance* frame, which vibrates at ~80 Hz and strobes at ~79–81 Hz.
(Refs: livescience.com/55996, thisiscolossal.com/2016/08/slow-dance-picture-frame-illusion)

## LED Strobe Driver — Q2

Same MOSFET and gate circuit as Q1.

**LED choice matters more here than anywhere else in the design.**

| Option | Part | Pros | Cons |
|--------|------|------|------|
| A | 1W Cree XP-E or similar SMD LED | Very bright, tiny, 3.3V Vf | Needs current resistor, soldering |
| B | 10W LED module (12V, COB) | Plug-and-play, very bright | Large, 12V so no resistor needed |
| C | Bright white 5mm LED (×10 array) | Dead simple, THT | Lower brightness |
| D | 12V LED strip segment | Easiest | Current limiting built in, moderate brightness |

**Recommendation: Option B (12V 10W COB module).** Plug straight in, extremely bright,
low duty cycle (1.5ms flash at 30 Hz = 4.5% duty) keeps average power ~450mW so it
runs cool indefinitely.

For a 10W LED module at 12V:
- No current-limiting resistor needed (module is designed for 12V)
- Peak current ~830 mA during flash — Q2 handles this easily

For a single high-power LED with known Vf (e.g., 3.3V Vf @ 350mA):
```
R = (12V - 3.3V) / 0.35A = 24.9Ω → use 22Ω 1W
```

## Power Budget

| Load               | Voltage | Avg Current (50% duty) | Power  |
|--------------------|---------|------------------------|--------|
| Electromagnet      | 12V     | ~250 mA average        | ~3W    |
| LED (10W module)   | 12V     | ~38 mA avg (4.5% duty) | ~0.5W  |
| ESP32 via LM317    | 12V     | ~150 mA                | ~1.8W (incl. ~1W LM317 heat) |
| Total              | 12V     | ~0.45A                 | ~5.3W  |

Note the LM317 is a **linear** regulator: it draws the same current on its 12V input as
the ESP32 pulls on the 5V side, burning the 12V−5.17V difference as heat (~1W at idle,
briefly more on WiFi transmit). A buck would have drawn ~half the 12V-side current — this
is the efficiency cost of the linear stage. It does not need a heatsink at this load.

A 12V **1A** supply is sufficient; the **6A** supply on hand is ample headroom.
The EM's **peak** current can hit 1–2A for the first few milliseconds on each cycle
(before the inductance limits it). The 12V supply needs to handle this — most wall
adapters can handle 2× rated current for brief spikes.

## Controls

All control is via the **web UI** (WiFi AP + HTTP) with a **serial command** fallback for
bench use. The analog potentiometers from the original design were dropped — they're
redundant with the web UI, and GPIO34/35 (input-only, no internal pull resistors) would
float and feed noise into the drive if left unpopulated while still being read.

## Bill of Materials

| Qty | Part                          | Approx Cost | Notes                          |
|-----|-------------------------------|-------------|--------------------------------|
| 1   | ESP32 DevKitC                 | ~$5         | Already have                   |
| 1   | 12V Electromagnet             | —           | Already have                   |
| 2   | IRLZ44N MOSFET (TO-220)       | $0.50 ea    | One for EM, one for LED        |
| 1   | 1N4001 rectifier diode        | $0.10 ea    | Flyback on EM coil             |
| 2   | 100Ω resistor ¼W              | <$0.05 ea   | Gate series                    |
| 2   | 10kΩ resistor ¼W              | <$0.05 ea   | Gate pull-downs                |
| 1   | LM317 regulator (TO-220)      | $0.30       | 12V→5.17V linear reg for ESP32 |
| 1   | 150Ω resistor ¼W              | <$0.05      | LM317 R1 (OUT→ADJ)             |
| 1   | 470Ω resistor ¼W              | <$0.05      | LM317 R2 (ADJ→GND)             |
| 2   | 470µF 50V electrolytic        | $0.30 ea    | 12V rail bulk decoupling       |
| 4   | 100nF ceramic cap             | $0.10 ea    | HF decoupling (Q1, Q2, LM317 in+out) |
| 1   | 10µF electrolytic             | $0.15 ea    | LM317 output / ESP32 VIN       |
| 1   | 12V 10W COB LED module        | $3–5        | Strobe light source            |
| 1   | 12V 6A power supply           | $5–10       | Wall adapter (6A on hand; 1–2A is plenty) |
| 1   | Breadboard or perfboard       | $2          | Assembly                       |
| —   | Wire, headers                 | ~$2         |                                |

**Estimated total for new parts: ~$15–25**
