#include <Arduino.h>
#include "config.h"
#include "drive.h"
#include "web_ui.h"

// ── Serial commands ───────────────────────────────────────────────────────────
// Simple single-char commands for bench use without a phone:
//   s = SYNC, m = SLOW_MO, e = EM_OFF, w = SWEEP
//   +/- = nudge phase ±5°, [/] = nudge freq ±0.5 Hz, </> = nudge EM duty ±5%
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
            case '>': drive_set_em_duty(drive_get_em_duty() + 5);
                      Serial.printf("em duty: %d%%\n", drive_get_em_duty()); break;
            case '<': drive_set_em_duty(drive_get_em_duty() - 5);
                      Serial.printf("em duty: %d%%\n", drive_get_em_duty()); break;
            case '?':
                Serial.printf("freq=%.1fHz  phase=%.0f°  pulse=%d  duty=%d%%  mode=%d\n",
                    drive_get_freq(), drive_get_phase(),
                    (int)drive_get_led_pulse(), (int)drive_get_em_duty(),
                    (int)drive_get_mode());
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
    Serial.printf("[strobe] %.1f Hz  phase=%.0f°  pulse=%d ticks  duty=%d%%  mode=%s\n",
        drive_get_freq(), drive_get_phase(),
        (int)drive_get_led_pulse(), (int)drive_get_em_duty(),
        mode_names[(int)drive_get_mode()]);
}

// ── Setup / loop ──────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(200);
    Serial.println("\n[strobe] booting…");

    drive_init();
    web_ui_init();

    Serial.printf("[strobe] ready  wifi=%s  ip=%s\n", WIFI_SSID, WIFI_IP);
    Serial.println("[strobe] serial cmds: s=sync m=slowmo e=emoff w=sweep +/-=phase [/]=freq </>=duty ?=status");
}

void loop() {
    unsigned long now = millis();

    handle_serial();
    web_ui_poll();

    // Advance sweep if in sweep mode
    drive_sweep_update(now);

    print_telemetry(now);

    delay(LOOP_MS);
}
