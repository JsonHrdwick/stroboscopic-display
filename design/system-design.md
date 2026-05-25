# System Design

## Core Idea

Two signals share a common timebase:
1. **EM drive** — square wave that energizes the electromagnet, pulling the reed tip down
2. **Strobe** — short pulse that fires the LED

The only parameters that matter are:
- `drive_freq_hz` — resonant frequency of the reed+subject system
- `strobe_freq_hz` — usually equal to drive_freq, but offset slightly for slow-motion
- `phase_deg` — where in the vibration cycle the "freeze" snapshot is taken
- `led_pulse_us` — flash duration (shorter = sharper freeze, dimmer; longer = blurrier, brighter)

## Signal Timing Diagram

```
EM drive:   ─────╔═══╗─────────╔═══╗─────────
                 │   │         │   │
                ON  OFF       ON  OFF
                 ← T_period →

Strobe:     ──────────────╔╗──────────────╔╗──
                           ││              ││
                         flash           flash
                 ←  phase_delay  →
                 ← T_period * (phase_deg/360) →

Reed tip:    ╱╲  ╱╲  ╱╲   (sinusoidal, resonant)
```

When strobe_freq == drive_freq and phase is constant, the LED always fires at the same
point in the vibration cycle → frozen image.

When strobe_freq is slightly lower (e.g., 29.9 Hz vs 30 Hz EM), the phase slips by
0.1 Hz relative to the vibration → subject appears to move very slowly.

## ESP32 Signal Generation

Two hardware timers (or two LEDC channels + one esp_timer for phase offset):

### Option A — Two Independent LEDC Channels (Simple)
- LEDC CH0: EM drive at drive_freq_hz, 50% duty
- LEDC CH1: Strobe at strobe_freq_hz, duty = led_pulse_us / period_us
- Phase offset NOT possible with LEDC alone — both channels share the same phase origin

### Option B — LEDC + esp_timer (Recommended)
- LEDC CH0: EM drive at drive_freq_hz, 50% duty
- Hardware Timer ISR: fires at drive_freq_hz, triggers esp_timer one-shot after phase_delay_us
- esp_timer callback: GPIO_LED HIGH for led_pulse_us, then LOW
- This gives independent phase control of the strobe relative to EM drive

### Option C — Single Hardware Timer (Maximum control)
- One timer ISR at drive_freq_hz × N (oversampled, e.g., N=360 for 1° resolution)
- ISR counts ticks, toggles EM at tick 0, fires LED pulse at tick `phase_deg`
- Most flexible but uses more CPU

**Recommendation: Option B.** Simple code, true phase control, minimal CPU overhead.

## GPIO Assignments

| GPIO | Function                       | Direction |
|------|--------------------------------|-----------|
| 5    | EM drive (LEDC CH0)            | OUTPUT    |
| 18   | LED strobe (esp_timer pulse)   | OUTPUT    |
| 34   | Frequency potentiometer (ADC)  | INPUT     |
| 35   | Phase potentiometer (ADC)      | INPUT     |
| 21   | SDA — optional OLED display    | I2C       |
| 22   | SCL — optional OLED display    | I2C       |

Using ADC1 (GPIO 32–39 only) for pots because ADC2 conflicts with WiFi.

## Resonance Detection (Auto-tune Mode)

The ESP32 can sweep drive frequency and watch for maximum LED brightness feedback —
or more simply, a small microphone/piezo on the base can detect the fundamental frequency
amplitude. At resonance, vibration amplitude peaks sharply.

A simpler approach: user holds a phone flashlight on the subject while sweeping frequency
until the freeze effect sharpens. Mark that frequency on the encoder/dial.

Even simpler for v1: just sweep manually with a pot and listen/watch.

## Operating Modes

| Mode          | Description                                                     |
|---------------|-----------------------------------------------------------------|
| SYNC          | Strobe exactly matches EM freq, phase adjustable                |
| SLOW_MO       | Strobe offset from EM by ±0.1–2 Hz, makes subject crawl        |
| HARMONIC      | Strobe at ½× or 2× EM freq, shows multiple phase snapshots     |
| FREE_STROBE   | EM off, strobe at arbitrary frequency (use external vibration)  |
| SWEEP         | EM sweeps frequency range to find resonance                     |
