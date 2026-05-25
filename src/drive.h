#pragma once
#include <Arduino.h>

// ── Operating modes ───────────────────────────────────────────────────────────
enum class DriveMode {
    SYNC,      // strobe locked to EM drive with adjustable phase offset
    SLOW_MO,   // strobe runs slightly slower than EM — image drifts in slow motion
    EM_OFF,    // electromagnet silent; strobe runs free (illuminate external vibration)
    SWEEP,     // EM slowly sweeps frequency range — find resonance by eye
};

// ── Init / teardown ───────────────────────────────────────────────────────────
void drive_init();

// ── Parameter setters (safe to call from main loop / web callbacks) ───────────
void drive_set_freq(float hz);                // EM drive frequency
void drive_set_phase(float deg);              // strobe phase offset (0–360)
void drive_set_led_pulse(uint8_t ticks);      // flash duration in 100 µs ticks
void drive_set_slow_mo_delta(float hz);       // strobe lag below drive freq
void drive_set_mode(DriveMode mode);

// ── Getters ───────────────────────────────────────────────────────────────────
float      drive_get_freq();
float      drive_get_phase();
uint8_t    drive_get_led_pulse();
DriveMode  drive_get_mode();

// ── Sweep tick (call from main loop while in SWEEP mode) ─────────────────────
void drive_sweep_update(unsigned long now_ms);
