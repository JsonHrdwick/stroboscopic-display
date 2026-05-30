#pragma once

// ── GPIO ──────────────────────────────────────────────────────────────────────
#define PIN_EM          5     // electromagnet MOSFET gate  (LEDC / timer driven)
#define PIN_LED         18    // strobe LED MOSFET gate

// ── Timing ────────────────────────────────────────────────────────────────────
// Single hardware timer fires at TICK_HZ; all transitions are counted in ticks.
// 10 kHz → 100 µs resolution. Fine for 60–120 Hz drive frequencies.
#define TICK_HZ             10000

// Operating band is kept above the ~60 Hz flicker-fusion threshold so the strobe
// reads as continuous light (the "always on" Slow Dance effect) rather than a visible
// flicker. The slow-motion illusion comes from the small strobe-vs-drive offset
// (SLOW_MO_DELTA_DEFAULT_HZ), NOT from a low absolute frequency.
#define FREQ_MIN_HZ         60.0f
#define FREQ_MAX_HZ         120.0f
#define FREQ_DEFAULT_HZ     80.0f

#define PHASE_DEFAULT_DEG   0.0f

// LED flash duration in ticks (1 tick = 100 µs).
// 15 ticks = 1500 µs — bright enough, short enough for a sharp freeze.
#define LED_PULSE_TICKS_DEFAULT  15
#define LED_PULSE_TICKS_MIN       5   //  500 µs — sharp but dim
#define LED_PULSE_TICKS_MAX      50   // 5000 µs — bright but motion-blurred

// Slow-motion mode: strobe fires this many Hz below the EM drive.
// At 0.5 Hz delta the frozen image crawls through one full cycle in 2 seconds.
#define SLOW_MO_DELTA_DEFAULT_HZ  0.5f

// EM drive duty cycle: percent of each period the coil is energized (the "pull" window).
// 50% is the original symmetric square wave. Lower values give a shorter, harder impulse —
// useful for driving a stiff resonant reed at 80 Hz, where a brief well-timed kick can
// build more amplitude than a long pull while running the coil cooler.
#define EM_DUTY_DEFAULT_PCT  50
#define EM_DUTY_MIN_PCT       5
#define EM_DUTY_MAX_PCT      90

// ── WiFi AP ───────────────────────────────────────────────────────────────────
#include "credentials.h"          // defines WIFI_SSID and WIFI_PASS (gitignored)
#define WIFI_IP     "192.168.4.1"

// ── Misc ──────────────────────────────────────────────────────────────────────
#define SERIAL_BAUD 115200
#define LOOP_MS     50    // main loop update rate (20 Hz)
