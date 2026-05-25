#include "drive.h"
#include "config.h"
#include <driver/gpio.h>
#include <soc/gpio_reg.h>

// ── Fast GPIO macros (pins 0–31 only) ─────────────────────────────────────────
// Avoids gpio_set_level() overhead inside the ISR.
#define GPIO_HI(pin)  REG_WRITE(GPIO_OUT_W1TS_REG, 1UL << (pin))
#define GPIO_LO(pin)  REG_WRITE(GPIO_OUT_W1TC_REG, 1UL << (pin))

static portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// ── ISR-shared state (all volatile, updated atomically for 32-bit values) ────
static volatile uint32_t s_tick          = 0;
static volatile uint32_t s_period_ticks  = TICK_HZ / FREQ_DEFAULT_HZ;
static volatile uint32_t s_em_off_tick   = (TICK_HZ / FREQ_DEFAULT_HZ) / 2;
static volatile uint32_t s_led_on_tick   = 0;
static volatile uint32_t s_led_off_tick  = LED_PULSE_TICKS_DEFAULT;
static volatile bool     s_em_enabled    = true;

// Separate strobe timer state (SLOW_MO / EM_OFF modes)
static volatile uint32_t s_strobe_tick         = 0;
static volatile uint32_t s_strobe_period_ticks = TICK_HZ / FREQ_DEFAULT_HZ;
static volatile bool     s_use_strobe_timer    = false;
static volatile uint8_t  s_led_pulse_ticks     = LED_PULSE_TICKS_DEFAULT;

// ── Non-volatile app state ────────────────────────────────────────────────────
static float      s_freq_hz     = FREQ_DEFAULT_HZ;
static float      s_phase_deg   = PHASE_DEFAULT_DEG;
static float      s_delta_hz    = SLOW_MO_DELTA_DEFAULT_HZ;
static DriveMode  s_mode        = DriveMode::SYNC;

static hw_timer_t* s_timer = nullptr;

// Sweep state
static float         s_sweep_freq    = FREQ_MIN_HZ;
static unsigned long s_sweep_last_ms = 0;
static const float   SWEEP_STEP_HZ   = 0.5f;
static const unsigned long SWEEP_STEP_MS = 200;  // step every 200 ms → full sweep ~23 s

// ── Helper: recompute ISR params from app state ───────────────────────────────
static void update_isr_params() {
    uint32_t pt     = (uint32_t)(TICK_HZ / s_freq_hz);
    uint32_t em_off = pt / 2;
    uint32_t led_on = (uint32_t)(s_phase_deg / 360.0f * (float)pt);
    uint32_t led_off = (led_on + s_led_pulse_ticks) % pt;
    if (led_off == led_on) led_off = (led_on + 1) % pt;  // guarantee at least 1 tick on

    // Strobe timer (SLOW_MO / EM_OFF): separate period, phase ignored
    float strobe_hz  = (s_mode == DriveMode::SLOW_MO)
                        ? (s_freq_hz - s_delta_hz)
                        : s_freq_hz;
    strobe_hz = constrain(strobe_hz, FREQ_MIN_HZ, FREQ_MAX_HZ);
    uint32_t spt = (uint32_t)(TICK_HZ / strobe_hz);

    portENTER_CRITICAL_ISR(&timerMux);
    s_period_ticks        = pt;
    s_em_off_tick         = em_off;
    s_led_on_tick         = led_on;
    s_led_off_tick        = led_off;
    s_strobe_period_ticks = spt;
    portEXIT_CRITICAL_ISR(&timerMux);
}

// ── Hardware timer ISR ─────────────────────────────────────────────────────────
// Fires at TICK_HZ (10 kHz). All EM and LED switching happens here.
// Two independent counters (s_tick for EM, s_strobe_tick for LED when decoupled).
void IRAM_ATTR drive_isr() {
    // ── EM drive (always from s_tick) ────────────────────────────────────────
    if (s_tick == 0 && s_em_enabled) {
        GPIO_HI(PIN_EM);
    }
    if (s_tick == s_em_off_tick || !s_em_enabled) {
        GPIO_LO(PIN_EM);
    }

    // ── Strobe LED ────────────────────────────────────────────────────────────
    if (!s_use_strobe_timer) {
        // SYNC mode: LED is phase-locked to s_tick
        if (s_tick == s_led_on_tick) {
            GPIO_HI(PIN_LED);
        } else if (s_tick == s_led_off_tick) {
            GPIO_LO(PIN_LED);
        }
    } else {
        // SLOW_MO / EM_OFF: LED runs on its own counter
        if (s_strobe_tick == 0) {
            GPIO_HI(PIN_LED);
        } else if (s_strobe_tick == s_led_pulse_ticks) {
            GPIO_LO(PIN_LED);
        }
        if (++s_strobe_tick >= s_strobe_period_ticks) s_strobe_tick = 0;
    }

    // ── Advance EM tick ───────────────────────────────────────────────────────
    if (++s_tick >= s_period_ticks) s_tick = 0;
}

// ── Public API ────────────────────────────────────────────────────────────────

void drive_init() {
    // Configure output pins
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = (1ULL << PIN_EM) | (1ULL << PIN_LED);
    cfg.mode         = GPIO_MODE_OUTPUT;
    cfg.pull_up_en   = GPIO_PULLUP_DISABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type    = GPIO_INTR_DISABLE;
    gpio_config(&cfg);
    GPIO_LO(PIN_EM);
    GPIO_LO(PIN_LED);

    update_isr_params();

    // Hardware timer: prescaler 8 on 80 MHz APB → 10 MHz tick clock
    // Alarm every (10 MHz / TICK_HZ) = 1000 ticks → fires at 10 kHz
    s_timer = timerBegin(0, 8, true);
    timerAttachInterrupt(s_timer, &drive_isr, true);
    timerAlarmWrite(s_timer, 10000000UL / TICK_HZ, true);
    timerAlarmEnable(s_timer);
}

void drive_set_freq(float hz) {
    s_freq_hz = constrain(hz, FREQ_MIN_HZ, FREQ_MAX_HZ);
    update_isr_params();
}

void drive_set_phase(float deg) {
    s_phase_deg = fmod(deg, 360.0f);
    if (s_phase_deg < 0) s_phase_deg += 360.0f;
    update_isr_params();
}

void drive_set_led_pulse(uint8_t ticks) {
    s_led_pulse_ticks = constrain(ticks, LED_PULSE_TICKS_MIN, LED_PULSE_TICKS_MAX);
    update_isr_params();
}

void drive_set_slow_mo_delta(float hz) {
    s_delta_hz = constrain(hz, 0.05f, 5.0f);
    update_isr_params();
}

void drive_set_mode(DriveMode mode) {
    s_mode = mode;
    switch (mode) {
        case DriveMode::SYNC:
            s_em_enabled      = true;
            s_use_strobe_timer = false;
            break;
        case DriveMode::SLOW_MO:
            s_em_enabled      = true;
            s_use_strobe_timer = true;
            break;
        case DriveMode::EM_OFF:
            s_em_enabled      = false;
            s_use_strobe_timer = true;
            GPIO_LO(PIN_EM);
            break;
        case DriveMode::SWEEP:
            s_em_enabled      = true;
            s_use_strobe_timer = false;
            s_sweep_freq      = FREQ_MIN_HZ;
            break;
    }
    update_isr_params();
}

void drive_sweep_update(unsigned long now_ms) {
    if (s_mode != DriveMode::SWEEP) return;
    if (now_ms - s_sweep_last_ms < SWEEP_STEP_MS) return;
    s_sweep_last_ms = now_ms;

    s_sweep_freq += SWEEP_STEP_HZ;
    if (s_sweep_freq > FREQ_MAX_HZ) s_sweep_freq = FREQ_MIN_HZ;

    s_freq_hz = s_sweep_freq;
    update_isr_params();
}

float     drive_get_freq()      { return s_freq_hz; }
float     drive_get_phase()     { return s_phase_deg; }
uint8_t   drive_get_led_pulse() { return s_led_pulse_ticks; }
DriveMode drive_get_mode()      { return s_mode; }
