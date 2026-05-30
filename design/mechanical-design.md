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

**Target ~80 Hz.** The reed resonance sets the whole system's operating frequency, and that
frequency must sit above the ~60 Hz flicker-fusion threshold so the strobe reads as
continuous light (see hardware-design.md). Jeff Lieberman's *Slow Dance* runs at ~80 Hz for
exactly this reason. Tune the reed to land in the **75–90 Hz** band — which means a shorter,
stiffer free length than a slow strobe would use.

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

Example — hacksaw blade tuned to the ~80 Hz target:
- h = 0.7 mm = 0.0007 m (thickness)
- L = 85 mm = 0.085 m (free length from clamp to EM contact point)
- f₀ ≈ 818 × (0.0007 / 0.007225) ≈ **79.3 Hz**

Shorter free length → higher frequency. Longer → lower. (A 120 mm free length on the
same blade drops to ~40 Hz — which would visibly flicker, so keep it short.)

Note the trade-off: a stiffer, shorter reed needs **more drive force** for the same tip
amplitude. The resonance Q buys most of that back (amplitude ≈ static deflection × Q), but
it makes electromagnet coupling and air gap more critical at 80 Hz than at 40 Hz.

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

Place the EM so its face is **~1 mm** below the reed tip at rest — as close as you can get
without the tip snapping into contact at full pull ("snap-through"). The air gap is the
single biggest lever on force: **magnetic force falls off roughly as 1/gap²**, so going from
3 mm to 1 mm can multiply the effective pull several-fold. At the 80 Hz target the reed is
stiff, so you need every bit of that force — start tight.

### Magnet coupling and force (critical at 80 Hz)

The force that matters here is **not** the magnet's rated "holding force" (measured at zero
gap, flush against a steel plate) — it's the force across your actual ~1 mm working gap with
the reed's small steel target. That can be a small fraction of the holding rating. To
maximize it:

- **Use a concentrated iron pole**, not a flat magnet face — a solenoid/coil with a
  protruding iron core focuses flux onto a small spot and pulls far better across a gap than
  a broad flat face. This is closer to how *Slow Dance* drives its reed.
- **Minimize the gap** (see above) and keep it consistent.
- **Drive in-phase**: pull while the reed is moving toward the magnet (firmware phase
  control) so each pulse adds kinetic energy — most effective near resonance.

If the reed tip is not ferromagnetic: glue a small **soft-steel** washer or disk (~6–8 mm
diameter, ~1 mm thick) to the underside of the reed tip to give the EM a good flux target.
Keep it light — added tip mass lowers f₀ (see Tip Mass Effect) and pulls you off 80 Hz.

### Damping is the enemy (subject choice affects force needed)

At resonance, achievable amplitude is set by how much energy damping removes each cycle.
A draggy subject (a fluffy feather, a wide ribbon) adds significant **air damping**, lowers
the system Q, and demands more drive force for visible motion. If the magnet struggles,
try a lower-drag subject (thin mylar strip, fine thread) before assuming you need a bigger
magnet — and only upgrade the magnet if the reed still won't move at minimum gap, on exact
resonance, with a low-drag subject.

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
