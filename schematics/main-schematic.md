# Main Schematic (ASCII)

```
                         12V 2A INPUT
                              │
           ┌──────────────────┼──────────────────────┐
           │                  │                      │
         C1│                C2│                 MP1584 Buck
      470µF               470µF               12V → 5V
           │                  │                      │
          GND                GND              ESP32 VIN
                                         (internal LDO → 3.3V)


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
  VIN     ← 5V from buck converter
  GND     ← all grounds common


────────────────── GROUND RULES ─────────────────────────────────

  Single star ground: PSU(-), Buck GND, Q1 Source, Q2 Source,
  ESP32 GND all meet at one point.

  12V current path (EM pulses) must not share a trace with GPIO/ADC
  wiring. Run EM return current directly back to PSU minus.
```

## Decoupling

| Location                  | Capacitor           |
|---------------------------|---------------------|
| 12V rail near Q1 Drain    | 470µF 50V + 100nF   |
| 12V rail near Q2 Drain    | 470µF 50V + 100nF   |
| ESP32 VIN                 | 10µF + 100nF        |

The 470µF caps absorb the inrush current when the EM switches on, preventing the
12V rail from drooping and causing the ESP32 to reset. Place the ceramic closest to
the MOSFET drain loop, electrolytic just behind it.
