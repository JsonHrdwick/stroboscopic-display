#pragma once

// ── GPIO ──────────────────────────────────────────────────────────────────────
#define PIN_EM          5     // electromagnet MOSFET gate  (LEDC / timer driven)
#define PIN_LED         18    // strobe LED MOSFET gate
#define PIN_POT_FREQ    34    // ADC1 ch6 — frequency pot   (must be ADC1, 32–39)
#define PIN_POT_PHASE   35    // ADC1 ch7 — phase pot

// ── Timing ────────────────────────────────────────────────────────────────────
// Single hardware timer fires at TICK_HZ; all transitions are counted in ticks.
// 10 kHz → 100 µs resolution. Fine for 5–120 Hz drive frequencies.
#define TICK_HZ             10000

#define FREQ_MIN_HZ         5.0f
#define FREQ_MAX_HZ         120.0f
#define FREQ_DEFAULT_HZ     30.0f

#define PHASE_DEFAULT_DEG   0.0f

// LED flash duration in ticks (1 tick = 100 µs).
// 15 ticks = 1500 µs — bright enough, short enough for a sharp freeze.
#define LED_PULSE_TICKS_DEFAULT  15
#define LED_PULSE_TICKS_MIN       5   //  500 µs — sharp but dim
#define LED_PULSE_TICKS_MAX      50   // 5000 µs — bright but motion-blurred

// Slow-motion mode: strobe fires this many Hz below the EM drive.
// At 0.5 Hz delta the frozen image crawls through one full cycle in 2 seconds.
#define SLOW_MO_DELTA_DEFAULT_HZ  0.5f

// ── ADC ───────────────────────────────────────────────────────────────────────
#define ADC_SAMPLES     16    // oversample to reduce noise
#define ADC_MAX         4095

// ── WiFi AP ───────────────────────────────────────────────────────────────────
#define WIFI_SSID   "StrobeDisplay"
#define WIFI_PASS   ""            // open AP — set a password if desired
#define WIFI_IP     "192.168.4.1"

// ── Misc ──────────────────────────────────────────────────────────────────────
#define SERIAL_BAUD 115200
#define LOOP_MS     50    // main loop update rate (20 Hz)
