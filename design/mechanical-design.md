# Mechanical Design

## Overview

The mechanical system is a driven cantilever. The electromagnet pulls the free end of a
flexible reed downward on each drive pulse. If the drive frequency matches the reed's
natural resonant frequency, amplitude builds up significantly (Q factor amplification).

```
Side view:

  clamp
  ┌────┐
  │    ├───────────────────────────── reed tip ~~~ feather
  └────┘
  (fixed)       ↑
            electromagnet
            (pulls tip down)
```

## Reed Selection

The resonant frequency of the system depends on the reed material, length, cross-section,
and tip mass (feather weight).

### Recommended Reed Materials

| Material              | f₀ guidance (150mm, no tip mass) | Notes                      |
|-----------------------|----------------------------------|----------------------------|
| Hacksaw blade         | 40–80 Hz                         | Best starting point        |
| Steel ruler (1mm)     | 15–40 Hz                         | Easy to find, adjustable   |
| Spring steel strip    | 20–100 Hz (varies by spec)       | Most controllable          |
| Guitar string (wound) | 30–200 Hz                        | Very light, needs a clamp  |
| Plastic ruler         | 5–20 Hz                          | Too damped, poor resonance |
| Wooden chopstick      | 10–30 Hz                         | Low Q, still works         |

**Start with a hacksaw blade.** Steel, high Q (low damping), easy to clamp, tunable by
adjusting the free length.

### Resonant Frequency Formula (cantilever beam)

```
f₀ = (λ₁²/2π) × √(EI / ρAL⁴)
```

Where λ₁ = 1.875 for the first mode. Simplified for a rectangular steel beam:

```
f₀ ≈ 0.162 × (h / L²) × √(E/ρ)

For steel: E = 200 GPa, ρ = 7850 kg/m³ → √(E/ρ) ≈ 5048 m/s

f₀ ≈ 818 × (h / L²)    [h and L in meters]
```

Example — hacksaw blade:
- h = 0.7 mm = 0.0007 m (thickness)
- L = 120 mm = 0.12 m (free length from clamp to EM contact point)
- f₀ ≈ 818 × (0.0007 / 0.0144) ≈ **39.8 Hz**

Shorter free length → higher frequency. Longer → lower.

**Practical tuning:** slide the clamp along the blade. Each cm of free length changes the
frequency by several Hz. Find the resonance by slowly sweeping the EM drive frequency until
the feather tip visibly jumps to maximum amplitude.

### Tip Mass Effect

Adding tip mass lowers frequency:
```
f_loaded = f₀ / √(1 + m_tip / (0.236 × m_beam))
```

A feather weighs ~0.05–0.3g — negligible vs. a hacksaw blade segment (~10g), so tip mass
effect is small. A heavier subject (e.g., a strip of aluminum foil) will noticeably lower
the resonant frequency.

## Electromagnet Position

The EM should be positioned below the free end of the reed, with a small air gap.

```
Top view (EM placement options):

  Option A — tip drive (maximum amplitude, less force leverage):
  ┌────┐
  │    ├──────────────────────────── tip
  └────┘                             ↑
                                     EM

  Option B — mid-span drive (more force, less tip amplitude):
  ┌────┐
  │    ├──────────────┬──────────── tip
  └────┘              ↑
                      EM
```

**Option A is better for demonstrations** — maximum visible tip amplitude, even though it
requires more magnetic force (longer lever arm means the EM needs to pull harder).

Place the EM so its face is 1–3 mm below the reed tip at rest. Too close and the tip
gets stuck at maximum pull (snap-through). Too far and not enough force.

If the reed tip is not ferromagnetic: glue a small steel washer or disk (~5mm diameter,
1mm thick) to the underside of the reed tip to give the EM something to attract.

## Mounting / Frame

### Minimal version (breadboard prototype)
```
┌─────────────────────────────────────────────────┐
│  Wooden base (200×150 mm plywood, 12mm thick)   │
│                                                  │
│  [Binder clip]  ←→ adjustable clamp position   │
│     ├── hacksaw blade (reed)                     │
│                                                  │
│  [Electromagnet] on a small wooden block         │
│     (height-adjustable with shims / tape)        │
│                                                  │
│  [LED] on a bent wire stand, aimed at subject   │
│                                                  │
│  [Electronics] breadboard to side               │
└─────────────────────────────────────────────────┘
```

A binder clip clamped to the wooden base is enough for v1. Use the screw holes of the
binder clip handles for adjustment and to wire-tie it down.

### Permanent version
- 3D print a base plate with a blade slot and adjustable clamp
- Dovetail slot for EM mounting bracket (sliding adjustment)
- Built-in LED holder angled 30° down toward reed tip

## Subject (the Display Object)

| Subject           | Notes                                                    |
|-------------------|----------------------------------------------------------|
| Feather           | Classic, very low mass, beautiful motion pattern         |
| Mylar strip       | Reflective, very visible under strobe, crinkles nicely   |
| Paper strip       | Easiest, shows clear wave modes                          |
| Rubber band       | Higher damping, shows standing waves at harmonic modes   |
| Thread/yarn       | Multiple nodes visible at higher harmonics               |
| Dried grass stem  | Organic look, lightweight                                |
| Thin ribbon       | Extremely clear, pairs well with UV LED                  |

Attach the subject to the reed tip with a small clip or tape. The mass should be
as low as possible to avoid shifting the resonant frequency significantly.

## LED Positioning

The LED strobe should be aimed at the subject from slightly above and to the side, not
directly head-on. A 30–45° angle gives the best depth contrast in the frozen image.

For maximum visual impact, work in a dark or dim room. The dark intervals between flashes
read as black; the brief flash is the only illumination.

Optional: UV (365 nm) LED + fluorescent-dyed feather or ribbon for a dramatic effect.
