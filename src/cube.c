#include "cube.h"
#include "math.h"
#include "zoom.h"
#include "imu.h"
#include "sd_card.h"        // sd_init_driver()
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico/platform.h"  // tight_loop_contents()

#include <stdio.h>
#include <string.h>


// ---------------------------------------------------------------------------
// HARDWARE
// ---------------------------------------------------------------------------
#define IMU_I2C_PORT  i2c0
#define IMU_SDA_PIN   4
#define IMU_SCL_PIN   5
#define IMU_BAUD      400000   // 400 kHz fast-mode

#define BTN_HOME_PIN  20   // Button 1: jump to isometric home view + unlock
#define BTN_LOCK_PIN  21   // Button 2: toggle view lock on/off

// ---------------------------------------------------------------------------
// PROJECTION
// ---------------------------------------------------------------------------
#define PROJ_FOV        300.0f
#define PROJ_DISTANCE     2.0f

#define ACCEL_SCALE  256.0f

#define HOME_ANGLE_X   45.0f
#define HOME_ANGLE_Y  -35.264f
#define HOME_ANGLE_Z    0.0f

#define DRAW_COLOR  15

// ---------------------------------------------------------------------------
// STATE
// ---------------------------------------------------------------------------
static SimpleModel s_model;
static i2c_inst_t *s_i2c;
static IMUReading  s_cal;

static bool        s_locked         = false;
static float       s_locked_angle_x = HOME_ANGLE_X;
static float       s_locked_angle_y = HOME_ANGLE_Y;

// Button debounce (pull-up idle = high = true)
static bool     s_btn_home_prev = true;
static bool     s_btn_lock_prev = true;
static uint32_t s_btn_home_ms   = 0;
static uint32_t s_btn_lock_ms   = 0;
#define DEBOUNCE_MS 50

// ---------------------------------------------------------------------------
// ERROR
// ---------------------------------------------------------------------------
static void halt_with_error(const char *msg) {
    printf("FATAL: %s\n", msg);
    while (1) tight_loop_contents();
}

// ---------------------------------------------------------------------------
// BUTTON
// ---------------------------------------------------------------------------
static bool button_fell(uint pin, bool *prev, uint32_t *last_ms) {
    bool now = gpio_get(pin);
    uint32_t t = to_ms_since_boot(get_absolute_time());

    if (!now && *prev && (t - *last_ms) > DEBOUNCE_MS) {
        *prev = false;
        *last_ms = t;
        return true;
    }
    if (now) *prev = true;
    return false;
}

// ---------------------------------------------------------------------------
// BINARY LOADER
// ---------------------------------------------------------------------------
static bool simple_model_load_bin(const char *path, SimpleModel *model) {
    FIL file;
    UINT br;

    if (f_open(&file, path, FA_READ) != FR_OK) return false;

    // Read counts
    if (f_read(&file, &model->num_vertices, sizeof(int), &br) != FR_OK) return false;
    if (f_read(&file, &model->num_edges, sizeof(int), &br) != FR_OK) return false;

    if (model->num_vertices > MAX_VERTICES || model->num_edges > MAX_EDGES) {
        f_close(&file);
        return false;
    }

    // Read vertices
    if (f_read(&file,
               model->vertices,
               sizeof(Point3D) * model->num_vertices,
               &br) != FR_OK) return false;

    // Read edges
    if (f_read(&file,
               model->edges,
               sizeof(int) * 2 * model->num_edges,
               &br) != FR_OK) return false;

    // Validate edge indices are within the loaded vertex range
    for (int i = 0; i < model->num_edges; i++) {
        if ((unsigned)model->edges[i][0] >= (unsigned)model->num_vertices ||
            (unsigned)model->edges[i][1] >= (unsigned)model->num_vertices) {
            f_close(&file);
            return false;
        }
    }

    f_close(&file);
    return true;
}

// ---------------------------------------------------------------------------
// CUBE INITIALIZE
// ---------------------------------------------------------------------------
void cube_init(void) {

    // ---- SD CARD ----
    if (!sd_init_driver()) {
        halt_with_error("SD init failed");
    }

    if (!simple_model_load_bin(MODEL_PATH, &s_model)) {
        halt_with_error("Failed to load model.bin");
    }

    // ---- IMU ----
    s_i2c = IMU_I2C_PORT;
    imu_init(s_i2c, IMU_SDA_PIN, IMU_SCL_PIN, IMU_BAUD);
    s_cal = imu_calibrate(s_i2c, 200);

    // ---- BUTTONS ----
    gpio_init(BTN_HOME_PIN);
    gpio_set_dir(BTN_HOME_PIN, GPIO_IN);
    gpio_pull_up(BTN_HOME_PIN);

    gpio_init(BTN_LOCK_PIN);
    gpio_set_dir(BTN_LOCK_PIN, GPIO_IN);
    gpio_pull_up(BTN_LOCK_PIN);

    zoom_init();
}

// ---------------------------------------------------------------------------
// CUBE RUN
// ---------------------------------------------------------------------------
void cube_run(void) {
    float angle_x = HOME_ANGLE_X;
    float angle_y = HOME_ANGLE_Y;
    float angle_z = HOME_ANGLE_Z;

    while (1) {

        // ---- BUTTONS ----
        if (button_fell(BTN_HOME_PIN, &s_btn_home_prev, &s_btn_home_ms)) {
            angle_x = HOME_ANGLE_X;
            angle_y = HOME_ANGLE_Y;
            angle_z = HOME_ANGLE_Z;
            s_locked = false;
        }

        if (button_fell(BTN_LOCK_PIN, &s_btn_lock_prev, &s_btn_lock_ms)) {
            s_locked = !s_locked;
            if (s_locked) {
                s_locked_angle_x = angle_x;
                s_locked_angle_y = angle_y;
            }
        }

        // ---- IMU ----
        if (!s_locked) {
            IMUReading data = imu_read_accel(s_i2c, &s_cal, 10);
            angle_x = data.ax / ACCEL_SCALE;
            angle_y = data.ay / ACCEL_SCALE;
        } else {
            angle_x = s_locked_angle_x;
            angle_y = s_locked_angle_y;
        }

        // ---- TRANSFORM ----
        Point3D projected[MAX_VERTICES];

        float dynamic_fov = zoom_get_fov();


        for (int i = 0; i < s_model.num_vertices; i++) {
            Point3D r = s_model.vertices[i];
            r = point3d_rotate_x(r, angle_x);
            r = point3d_rotate_y(r, angle_y);
            r = point3d_rotate_z(r, angle_z);
            
            projected[i] = point3d_project(r, SCREEN_WIDTH, SCREEN_HEIGHT, dynamic_fov, PROJ_DISTANCE);
        }

        // ---- RENDER ----
        clear_screen();

        for (int i = 0; i < s_model.num_edges; i++) {
            Point3D *a = &projected[s_model.edges[i][0]];
            Point3D *b = &projected[s_model.edges[i][1]];

            draw_line((int)a->x, (int)a->y,
                      (int)b->x, (int)b->y,
                      DRAW_COLOR);
        }
    }
}