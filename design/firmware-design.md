# Firmware Design

## Platform
PlatformIO + Arduino framework, ESP32.

## Architecture

This is much simpler than the rotary version. No interrupts for sensing, no PID.
Just two output signals from the same clock source.

```
ADC (pot 1) → drive_freq_hz
ADC (pot 2) → phase_deg

LEDC timer → EM drive square wave at drive_freq_hz
             50% duty (on for half period, off for half)

Hardware Timer ISR → fires at drive_freq_hz
  └── starts esp_timer one-shot delayed by (phase_deg/360 × period_us)
      └── led_on_cb → GPIO_LED HIGH
          └── (led_pulse_us later) → GPIO_LED LOW
```

## File Structure

```
src/
├── main.cpp        ← setup(), loop(), task creation
├── config.h        ← pin defs and tuning constants
├── em_drive.cpp    ← LEDC setup, frequency changes
├── em_drive.h
├── strobe.cpp      ← phase-delayed LED pulse generation
├── strobe.h
└── web_ui.cpp      ← optional WiFi config interface
    web_ui.h
```

## config.h

```cpp
#define PIN_EM          5
#define PIN_LED         18
#define PIN_POT_FREQ    34   // ADC1 channel 6
#define PIN_POT_PHASE   35   // ADC1 channel 7

#define LEDC_CH_EM      0
#define LEDC_RESOLUTION 14   // 14-bit: 0–16383 counts, good freq resolution

#define FREQ_MIN_HZ     5.0f
#define FREQ_MAX_HZ     120.0f
#define LED_PULSE_US    1500   // strobe flash duration — adjust for brightness/blur tradeoff

#define ADC_SAMPLES     8      // oversample ADC to reduce noise
```

## em_drive.cpp

```cpp
void em_init() {
    ledcSetup(LEDC_CH_EM, 30.0, LEDC_RESOLUTION);
    ledcAttachPin(PIN_EM, LEDC_CH_EM);
    // 50% duty = half of max counts
    ledcWrite(LEDC_CH_EM, (1 << LEDC_RESOLUTION) / 2);
}

void em_set_freq(float hz) {
    hz = constrain(hz, FREQ_MIN_HZ, FREQ_MAX_HZ);
    ledcWriteTone(LEDC_CH_EM, hz);
    // ledcWriteTone sets 50% duty automatically
}
```

## strobe.cpp

```cpp
// The strobe must fire once per EM cycle, at a phase offset.
// We use a hardware timer ISR that fires at drive_freq_hz.
// Inside the ISR we schedule a one-shot esp_timer for the phase delay.

static volatile float g_phase_deg = 0.0f;
static volatile float g_period_us = 33333.0f;  // 1/30Hz in µs
static esp_timer_handle_t led_on_timer;
static esp_timer_handle_t led_off_timer;

void IRAM_ATTR strobe_sync_isr() {
    // This ISR fires at drive_freq_hz from the hw_timer
    uint32_t delay_us = (uint32_t)(g_phase_deg / 360.0f * g_period_us);
    esp_timer_start_once(led_on_timer, delay_us);
}

void led_on_cb(void* arg) {
    gpio_set_level(PIN_LED, 1);
    esp_timer_start_once(led_off_timer, LED_PULSE_US);
}

void led_off_cb(void* arg) {
    gpio_set_level(PIN_LED, 0);
}

void strobe_init() {
    gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_LED, 0);

    esp_timer_create_args_t on_args  = { .callback = led_on_cb };
    esp_timer_create_args_t off_args = { .callback = led_off_cb };
    esp_timer_create(&on_args, &led_on_timer);
    esp_timer_create(&off_args, &led_off_timer);

    // hw_timer fires at drive_freq_hz — attach strobe_sync_isr
    hw_timer_t* timer = timerBegin(0, 80, true);  // 80 prescaler = 1µs tick
    timerAttachInterrupt(timer, &strobe_sync_isr, true);
    timerAlarmWrite(timer, (uint32_t)g_period_us, true);
    timerAlarmEnable(timer);
}

void strobe_set_freq(float hz) {
    g_period_us = 1000000.0f / hz;
    timerAlarmWrite(strobe_timer, (uint32_t)g_period_us, true);
}

void strobe_set_phase(float deg) {
    g_phase_deg = constrain(deg, 0.0f, 359.9f);
}
```

## main.cpp

```cpp
void setup() {
    Serial.begin(115200);
    em_init();
    strobe_init();
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);  // full 0–3.3V range
}

void loop() {
    // Read and smooth potentiometers
    float freq  = read_pot_freq();    // returns FREQ_MIN–FREQ_MAX
    float phase = read_pot_phase();   // returns 0–360

    em_set_freq(freq);
    strobe_set_freq(freq);            // change to strobe_set_freq(freq * 0.99) for slow-mo
    strobe_set_phase(phase);

    // Optional: serial telemetry
    Serial.printf("freq=%.1f Hz  phase=%.1f°\n", freq, phase);
    delay(50);  // update rate: 20 Hz is plenty for pot reading
}

float read_pot_freq() {
    uint32_t sum = 0;
    for (int i = 0; i < ADC_SAMPLES; i++) sum += analogRead(PIN_POT_FREQ);
    float val = sum / (float)ADC_SAMPLES;
    return FREQ_MIN_HZ + (val / 4095.0f) * (FREQ_MAX_HZ - FREQ_MIN_HZ);
}

float read_pot_phase() {
    uint32_t sum = 0;
    for (int i = 0; i < ADC_SAMPLES; i++) sum += analogRead(PIN_POT_PHASE);
    return (sum / (float)(ADC_SAMPLES * 4095)) * 360.0f;
}
```

## Sweep Mode (Resonance Finding)

Add a 3rd mode triggered by a button or serial command:

```cpp
void sweep_resonance() {
    // Sweep from FREQ_MIN to FREQ_MAX over 10 seconds
    for (float f = FREQ_MIN_HZ; f <= FREQ_MAX_HZ; f += 0.1f) {
        em_set_freq(f);
        strobe_set_freq(f);
        Serial.printf("sweeping %.1f Hz\n", f);
        delay(100);
    }
    // User watches for maximum feather amplitude
    // Then rotates frequency pot to that value
}
```

## platformio.ini

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
monitor_filters = esp32_exception_decoder
build_flags =
    -DCORE_DEBUG_LEVEL=2
```

No external libraries needed for the core functionality.

## Notes on LEDC vs Manual Timer for EM

LEDC is the ESP32's hardware PWM peripheral. It can generate precise square waves down
to ~1 Hz without CPU involvement. There is one caveat: when you call `ledcWriteTone()`,
there is a brief glitch in the output. This is fine because the glitch is infrequent
(only when the pot moves) and lasts < 1 ms.

The hardware timer for the strobe sync needs to match the LEDC frequency exactly to avoid
phase drift. Update both atomically (disable interrupts briefly during the update) or accept
a slow drift that re-locks within a few cycles.

For the most robust sync: derive both signals from the same hardware timer (Option C from
system-design.md). The LEDC approach (Option B) is fine for casual use.
