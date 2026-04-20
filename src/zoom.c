#include "zoom.h"
#include "hardware/adc.h"

// ---------------------------------------------------------------------------
// ADC CONFIGURATION
// ---------------------------------------------------------------------------
#define ZOOM_ADC_GPIO   45
#define ZOOM_ADC_INPUT   5

// ---------------------------------------------------------------------------
// FIELD OF VIEW RANGE
// ---------------------------------------------------------------------------
#define FOV_MIN   100.0f
#define FOV_MAX   380.0f

// ---------------------------------------------------------------------------
// SMOOTHING FACTOR
// ---------------------------------------------------------------------------
#define SMOOTH_ALPHA  0.9f

// ---------------------------------------------------------------------------
// INITIALIZES THE ZOOM
// ---------------------------------------------------------------------------
void zoom_init(void) {
    adc_init();
    adc_gpio_init(ZOOM_ADC_GPIO);
    adc_select_input(ZOOM_ADC_INPUT);
}

// ---------------------------------------------------------------------------
// TURNS ADC VALUE INTO FOV VALUE
// ---------------------------------------------------------------------------
float zoom_get_fov(void) {

    // ---- Step 1: Read ADC ----
    uint16_t raw = adc_read();   // 12-bit value

    // ---- Step 2: Normalize ----
    float t = raw / 4095.0f;

    // ---- Step 3: Map to FOV range ----
    float target_fov = FOV_MIN + t * (FOV_MAX - FOV_MIN);

    // ---- Step 4: Smooth (low-pass filter) ----
    // float s_smooth_fov = SMOOTH_ALPHA * s_smooth_fov + (1.0f - SMOOTH_ALPHA) * target_fov;
    return target_fov;
}