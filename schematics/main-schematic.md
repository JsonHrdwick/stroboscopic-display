# Main Schematic (ASCII)

```
                         12V 6A INPUT
                              │
           ┌──────────────────┼──────────────────────┐
           │                  │                      │
         C1│                C2│                 LM317 5V reg
      470µF               470µF              (see 5V REGULATOR)
           │                  │                      │
          GND                GND              ESP32 VIN
                                         (internal LDO → 3.3V)


────────────────── 5V REGULATOR (LM317) ─────────────────────────

  12V ──┬──[IN]   LM317   [OUT]──┬──────┬──── 5.17V → ESP32 VIN
        │        (TO-220)        │      │
     100nF                    R1 150Ω  10µF
        │                        │      │
       GND                     [ADJ]    GND
                                 │
                              R2 470Ω
                                 │
                                GND

  Vout = 1.25 × (1 + R2/R1) = 1.25 × (1 + 470/150) ≈ 5.17 V
  Pin order (TO-220, facing label): ADJ · OUT · IN ; tab = OUT (5 V)
  Drops 12→5 V linearly: ~0.7 W at ESP32 idle. Runs hot (~65 °C) but
  safe bare — no heatsink needed at this light WiFi web-server load.
  R1 (150Ω) also sets the LM317's minimum load (~8 mA).


────────────────── ELECTROMAGNET DRIVE ──────────────────────────

  12V ─────────────────────────────────── EM (+)
                                           │
                                        EM coil
                                           │
  [1N4001 Cathode]──────────────────── EM (-)
  [1N4001 Anode] ────────────────────── Drain ─┐
                                                │
                                          Q1: IRLZ44N
                                             Gate ──[100Ω]── GPIO5 (ESP32)
                                             │
                                           [10kΩ]
                                             │
                                           Source
                                             │
                                            GND


────────────────── LED STROBE ───────────────────────────────────

  12V ──── LED (+) [10W COB module] ──── LED (-) ──────────────┐
                                                               │
                                                         Q2: IRLZ44N
                                                            Gate ──[100Ω]── GPIO18 (ESP32)
                                                            │
                                                          [10kΩ]
                                                            │
                                                          Source
                                                            │
                                                           GND


────────────────── OPTIONAL: OLED DISPLAY ───────────────────────

  ESP32 GPIO21 (SDA) ──── OLED SDA
  ESP32 GPIO22 (SCL) ──── OLED SCL
  ESP32 3.3V         ──── OLED VCC
  GND                ──── OLED GND


────────────────── PIN SUMMARY ──────────────────────────────────

  GPIO5   → Q1 Gate (EM drive, LEDC CH0)
  GPIO18  → Q2 Gate (LED strobe, esp_timer)
  GPIO21  ↔ OLED SDA (optional)
  GPIO22  ↔ OLED SCL (optional)
  VIN     ← 5.17V from LM317 regulator
  GND     ← all grounds common


────────────────── GROUND RULES ─────────────────────────────────

  Single star ground: PSU(-), LM317 GND, Q1 Source, Q2 Source,
  ESP32 GND all meet at one point.

  12V current path (EM pulses) must not share a trace with GPIO/ADC
  wiring. Run EM return current directly back to PSU minus.
```

## Decoupling

| Location                  | Capacitor           |
|---------------------------|---------------------|
| 12V rail near Q1 Drain    | 470µF 50V + 100nF   |
| 12V rail near Q2 Drain    | 470µF 50V + 100nF   |
| LM317 input (12V)         | 100nF               |
| LM317 output → ESP32 VIN  | 10µF + 100nF        |

The 470µF caps absorb the inrush current when the EM switches on, preventing the
12V rail from drooping and causing the ESP32 to reset. Place the ceramic closest to
the MOSFET drain loop, electrolytic just behind it.

The LM317 output 10µF absorbs the ESP32's WiFi-transmit current spikes (~350–500 mA
for a few ms) so VIN doesn't sag faster than the regulator can respond. Keep it close
to the regulator output / ESP32 VIN pin.
