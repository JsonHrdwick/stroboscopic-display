# Hardware Design

## Schematic Overview

```
12V PSU → [Buck 12V→5V] → ESP32 VIN
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

Use **1N5819 Schottky** (faster than 1N4007, recovers in < 10 ns vs. 2 µs):
```
12V ──[Cathode]──[1N5819]──[Anode]── MOSFET Drain (EM-)
```

### EM Drive Frequency & Core Saturation

Most 12V hobby electromagnets have an iron core. Iron saturates at ~1.5T, and at 50 Hz
the core losses start increasing. Most cheap electromagnets can be driven at 5–120 Hz
without visible heating, but watch the temperature — if the coil gets hot at your target
frequency, reduce the duty cycle (e.g., 30% instead of 50%).

The electromagnet fires **once per drive cycle** (pull on, release off). This means:
- At 30 Hz drive: 30 pulls/second → reed tip vibrates at 30 Hz
- The reed must be tuned so its resonant frequency matches the drive

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
| ESP32 + LDO losses | 5V      | 240 mA                 | 1.2W   |
| Total              | 12V     | ~0.7A                  | ~4.7W  |

A 12V **1A** supply is sufficient. A 2A supply gives comfortable headroom.
The EM's **peak** current can hit 1–2A for the first few milliseconds on each cycle
(before the inductance limits it). The 12V supply needs to handle this — most wall
adapters can handle 2× rated current for brief spikes.

## Optional: Analog Controls

Two 10kΩ potentiometers provide hands-on control without needing a phone or computer.

| Pot | GPIO | Controls         | Range       |
|-----|------|------------------|-------------|
| P1  | 34   | Drive frequency  | 5–120 Hz    |
| P2  | 35   | Phase offset     | 0–360°      |

ESP32 ADC is 12-bit (0–4095). Map:
- P1: `freq = map(adc_val, 0, 4095, 5, 120)` Hz
- P2: `phase = map(adc_val, 0, 4095, 0, 360)` degrees

**Note:** ESP32 ADC has a nonlinear response at the top and bottom 10% of range. Apply a
simple lookup-table correction or just accept the nonlinearity at extremes.

## Bill of Materials

| Qty | Part                          | Approx Cost | Notes                          |
|-----|-------------------------------|-------------|--------------------------------|
| 1   | ESP32 DevKitC                 | ~$5         | Already have                   |
| 1   | 12V Electromagnet             | —           | Already have                   |
| 2   | IRLZ44N MOSFET (TO-220)       | $0.50 ea    | One for EM, one for LED        |
| 2   | 1N5819 Schottky diode         | $0.15 ea    | Flyback on EM, protection      |
| 2   | 100Ω resistor ¼W              | <$0.05 ea   | Gate series                    |
| 2   | 10kΩ resistor ¼W              | <$0.05 ea   | Gate pull-downs                |
| 2   | 10kΩ pot                      | $0.50 ea    | Frequency and phase knobs      |
| 2   | 100µF 25V electrolytic        | $0.25 ea    | 12V rail decoupling            |
| 4   | 100nF ceramic cap             | $0.10 ea    | HF decoupling                  |
| 1   | 12V 10W COB LED module        | $3–5        | Strobe light source            |
| 1   | MP1584 buck module (12V→5V)   | $1–2        | Powers ESP32                   |
| 1   | 12V 2A power supply           | $5–10       | Wall adapter                   |
| 1   | Breadboard or perfboard       | $2          | Assembly                       |
| —   | Wire, headers                 | ~$2         |                                |

**Estimated total for new parts: ~$15–25**
