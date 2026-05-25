#include <Arduino.h>
#include "config.h"
#include "drive.h"
#include "web_ui.h"

// ── ADC helpers ───────────────────────────────────────────────────────────────
static uint32_t read_adc_avg(int pin) {
    uint32_t sum = 0;
    for (int i = 0; i < ADC_SAMPLES; i++) sum += analogRead(pin);
    return sum / ADC_SAMPLES;
}

static float map_float(float x, float in_lo, float in_hi, float out_lo, float out_hi) {
    return out_lo + (x - in_lo) * (out_hi - out_lo) / (in_hi - in_lo);
}

// ── Pot reading ───────────────────────────────────────────────────────────────
// Returns frequency in Hz, mapped with a sqrt curve so the low end
// (where resonance hunting happens) has finer resolution.
static float read_freq_pot() {
    float raw = (float)read_adc_avg(PIN_POT_FREQ) / ADC_MAX;
    raw = constrain(raw, 0.0f, 1.0f);
    // sqrt mapping: more resolution in 5–40 Hz range
    float norm = raw * raw;
    return map_float(norm, 0.0f, 1.0f, FREQ_MIN_HZ, FREQ_MAX_HZ);
}

static float read_phase_pot() {
    float raw = (float)read_adc_avg(PIN_POT_PHASE) / ADC_MAX;
    return map_float(constrain(raw, 0.0f, 1.0f), 0.0f, 1.0f, 0.0f, 359.9f);
}

// ── Serial commands ───────────────────────────────────────────────────────────
// Simple single-char commands for bench use without a phone:
//   s = SYNC, m = SLOW_MO, e = EM_OFF, w = SWEEP
//   +/- = nudge phase ±5°, [/] = nudge freq ±0.5 Hz
static void handle_serial() {
    while (Serial.available()) {
        char c = Serial.read();
        switch (c) {
            case 's': drive_set_mode(DriveMode::SYNC);    Serial.println("mode: SYNC");    break;
            case 'm': drive_set_mode(DriveMode::SLOW_MO); Serial.println("mode: SLOW_MO"); break;
            case 'e': drive_set_mode(DriveMode::EM_OFF);  Serial.println("mode: EM_OFF");  break;
            case 'w': drive_set_mode(DriveMode::SWEEP);   Serial.println("mode: SWEEP");   break;
            case '+': drive_set_phase(drive_get_phase() + 5.0f);
                      Serial.printf("phase: %.0f°\n", drive_get_phase()); break;
            case '-': drive_set_phase(drive_get_phase() - 5.0f);
                      Serial.printf("phase: %.0f°\n", drive_get_phase()); break;
            case ']': drive_set_freq(drive_get_freq() + 0.5f);
                      Serial.printf("freq: %.1f Hz\n", drive_get_freq()); break;
            case '[': drive_set_freq(drive_get_freq() - 0.5f);
                      Serial.printf("freq: %.1f Hz\n", drive_get_freq()); break;
            case '?':
                Serial.printf("freq=%.1fHz  phase=%.0f°  pulse=%d  mode=%d\n",
                    drive_get_freq(), drive_get_phase(),
                    (int)drive_get_led_pulse(), (int)drive_get_mode());
                break;
        }
    }
}

// ── Telemetry ─────────────────────────────────────────────────────────────────
static unsigned long s_telem_ms = 0;
static void print_telemetry(unsigned long now) {
    if (now - s_telem_ms < 2000) return;
    s_telem_ms = now;
    static const char* mode_names[] = { "SYNC", "SLOW_MO", "EM_OFF", "SWEEP" };
    Serial.printf("[strobe] %.1f Hz  phase=%.0f°  pulse=%d ticks  mode=%s\n",
        drive_get_freq(), drive_get_phase(),
        (int)drive_get_led_pulse(),
        mode_names[(int)drive_get_mode()]);
}

// ── Setup / loop ──────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(200);
    Serial.println("\n[strobe] booting…");

    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);  // full 0–3.3 V range on ADC pins

    drive_init();
    web_ui_init();

    Serial.printf("[strobe] ready  wifi=%s  ip=%s\n", WIFI_SSID, WIFI_IP);
    Serial.println("[strobe] serial cmds: s=sync m=slowmo e=emoff w=sweep +/-=phase [/]=freq ?=status");
}

void loop() {
    unsigned long now = millis();

    handle_serial();
    web_ui_poll();

    // Pots control freq and phase unless the web UI has sent a command recently.
    // Web override expires after WEB_OVERRIDE_TTL_MS so pots retake control naturally.
    if (!web_ui_has_override()) {
        DriveMode mode = drive_get_mode();
        if (mode == DriveMode::SYNC || mode == DriveMode::SLOW_MO) {
            drive_set_freq(read_freq_pot());
        }
        if (mode == DriveMode::SYNC) {
            drive_set_phase(read_phase_pot());
        }
    }

    // Advance sweep if in sweep mode
    drive_sweep_update(now);

    print_telemetry(now);

    delay(LOOP_MS);
}
