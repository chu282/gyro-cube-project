#include "cube.h"
#include "math.h"
#include "zoom.h"
#include "imu.h"
#include "cube_math.h"
#include "ff.h"
#include "diskio.h"
#include "sd_spi.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico/platform.h"  // tight_loop_contents()

#include <stdio.h>
#include <string.h>
#include "hardware/pio.h"
#include "vga.h"
#include <stdbool.h>

// ---------------------------------------------------------------------------
// HARDWARE
// ---------------------------------------------------------------------------
#define IMU_I2C_PORT  i2c0
#define IMU_SDA_PIN   4
#define IMU_SCL_PIN   5
#define IMU_BAUD      400000   // 400 kHz fast-mode

#define BTN_HOME_PIN  42   // Button 1: jump to isometric home view + unlock
#define BTN_LOCK_PIN  41   // Button 2: toggle view lock on/off

// ---------------------------------------------------------------------------
// PROJECTION
// ---------------------------------------------------------------------------
#define PROJ_FOV        300.0f
#define PROJ_DISTANCE     3.0f

#define ACCEL_SCALE  182.04f
#define DT 0.01f              // 10 ms loop (~100 Hz)
#define ALPHA 0.98f          // gyro weight

#define HOME_ANGLE_X    0.0f
#define HOME_ANGLE_Y    0.0f
#define HOME_ANGLE_Z    0.0f

#define DRAW_COLOR  15

// #define USE_HARDCODED_MODEL

// ---------------------------------------------------------------------------
// STATE
// ---------------------------------------------------------------------------
static SimpleModel s_model;
static i2c_inst_t *s_i2c;
static IMUReading  s_cal;

static bool s_locked = false;
float s_locked_angle_x;
float s_locked_angle_y;

int btn_home_last_time;
int btn_lock_last_time;
volatile bool btn_home_flag = false;
volatile bool btn_lock_flag = false;
#define DEBOUNCE_MS 200

extern uint vsync_sm;
extern PIO pio;
extern volatile bool vsync_flag;
extern uint8_t* front_buf;
extern uint8_t* back_buf;
extern uint8_t* address_pointer;

static float offset_x = 0.0f;
static float offset_y = 0.0f;

// ---------------------------------------------------------------------------
// ERROR
// ---------------------------------------------------------------------------
// static void halt_with_error(const char *msg) {
//     printf("FATAL: %s\n", msg);
//     while (1) tight_loop_contents();
// }

// ---------------------------------------------------------------------------
// BUTTON
// ---------------------------------------------------------------------------
// static bool button_fell(uint pin, bool *prev, uint32_t *last_ms) {
//     bool now = gpio_get(pin);
//     uint32_t t = to_ms_since_boot(get_absolute_time());

//     if (!now && *prev && (t - *last_ms) > DEBOUNCE_MS) {
//         *prev = false;
//         *last_ms = t;
//         return true;
//     }
//     if (now) *prev = true;
//     return false;
// }

// ---------------------------------------------------------------------------
// TEST MODEL SWITCH
// Define USE_HARDCODED_MODEL to skip the SD card entirely and use the
// built-in unit cube below.  Comment it out to load from model.bin instead.
// ---------------------------------------------------------------------------
// #define USE_HARDCODED_MODEL

#ifdef USE_HARDCODED_MODEL
// Unit cube: 8 vertices at (±1, ±1, ±1), 12 edges.
// The projection in math.c adds SCREEN_WIDTH/2 and SCREEN_HEIGHT/2 as the
// screen-centre offset, so the cube sits in the middle of the display.
static void load_hardcoded_cube(SimpleModel *model) {
    model->num_vertices = 8;
    model->num_edges    = 12;

    // Vertices
    model->vertices[0] = (Point3D){ -1.0f, -1.0f, -1.0f };
    model->vertices[1] = (Point3D){  1.0f, -1.0f, -1.0f };
    model->vertices[2] = (Point3D){  1.0f,  1.0f, -1.0f };
    model->vertices[3] = (Point3D){ -1.0f,  1.0f, -1.0f };
    model->vertices[4] = (Point3D){ -1.0f, -1.0f,  1.0f };
    model->vertices[5] = (Point3D){  1.0f, -1.0f,  1.0f };
    model->vertices[6] = (Point3D){  1.0f,  1.0f,  1.0f };
    model->vertices[7] = (Point3D){ -1.0f,  1.0f,  1.0f };

    // Edges: back face, front face, then 4 connecting pillars
    model->edges[0][0] = 0; model->edges[0][1] = 1;   // back bottom
    model->edges[1][0] = 1; model->edges[1][1] = 2;   // back right
    model->edges[2][0] = 2; model->edges[2][1] = 3;   // back top
    model->edges[3][0] = 3; model->edges[3][1] = 0;   // back left

    model->edges[4][0] = 4; model->edges[4][1] = 5;   // front bottom
    model->edges[5][0] = 5; model->edges[5][1] = 6;   // front right
    model->edges[6][0] = 6; model->edges[6][1] = 7;   // front top
    model->edges[7][0] = 7; model->edges[7][1] = 4;   // front left

    model->edges[8][0]  = 0; model->edges[8][1]  = 4;  // pillar bottom-left
    model->edges[9][0]  = 1; model->edges[9][1]  = 5;  // pillar bottom-right
    model->edges[10][0] = 2; model->edges[10][1] = 6;  // pillar top-right
    model->edges[11][0] = 3; model->edges[11][1] = 7;  // pillar top-left
}

// static void load_hardcoded_cube(SimpleModel *model) {model->num_vertices = 5;
//     //Tetrahedron NOT Cube
//     model->num_edges    = 8;

//     // Vertices for the square base (resting at Y = -1.0)
//     model->vertices[0] = (Point3D){ -1.0f, -1.0f, -1.0f }; // Back-left
//     model->vertices[1] = (Point3D){  1.0f, -1.0f, -1.0f }; // Back-right
//     model->vertices[2] = (Point3D){  1.0f, -1.0f,  1.0f }; // Front-right
//     model->vertices[3] = (Point3D){ -1.0f, -1.0f,  1.0f }; // Front-left

//     // Vertex for the Apex (Tip of the pyramid, centered at Y = 1.0)
//     model->vertices[4] = (Point3D){  0.0f,  1.0f,  0.0f };

//     // Edges: The square base
//     model->edges[0][0] = 0; model->edges[0][1] = 1; // back edge
//     model->edges[1][0] = 1; model->edges[1][1] = 2; // right edge
//     model->edges[2][0] = 2; model->edges[2][1] = 3; // front edge
//     model->edges[3][0] = 3; model->edges[3][1] = 0; // left edge

//     // Edges: Connecting the base corners to the apex (vertex 4)
//     model->edges[4][0] = 0; model->edges[4][1] = 4; // back-left to tip
//     model->edges[5][0] = 1; model->edges[5][1] = 4; // back-right to tip
//     model->edges[6][0] = 2; model->edges[6][1] = 4; // front-right to tip
//     model->edges[7][0] = 3; model->edges[7][1] = 4; // front-left to tip
// }
#endif  // USE_HARDCODED_MODEL

// ---------------------------------------------------------------------------
// BINARY LOADER (used when USE_HARDCODED_MODEL is NOT defined)
// ---------------------------------------------------------------------------
#ifndef USE_HARDCODED_MODEL
static bool simple_model_load_bin(const char *path, SimpleModel *model) {
    FIL file;
    UINT br;
    FATFS fs;
    FRESULT fr;

    fr = f_mount(&fs, "", 1);
    if (fr != FR_OK) {
        printf("Mount failed: %d\n", fr);
        while (1);
    }

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
#endif  // !USE_HARDCODED_MODEL

// ---------------------------------------------------------------------------
// CUBE INITIALIZE
// ---------------------------------------------------------------------------
void cube_init(void) {

    // ---- MODEL LOAD ----
#ifdef USE_HARDCODED_MODEL
    load_hardcoded_cube(&s_model);          // no SD card needed
#else
    if (!simple_model_load_bin(MODEL_PATH, &s_model)) {
        printf("Failed to load model.");
    }
    else printf("SD Card successfully loaded.");
#endif

    // ---- IMU ----
    s_i2c = IMU_I2C_PORT;
    imu_init(s_i2c, IMU_SDA_PIN, IMU_SCL_PIN, IMU_BAUD);
    s_cal = imu_calibrate(s_i2c, 200);

    // ---- BUTTONS ----
    // gpio_init(BTN_HOME_PIN);
    // gpio_set_dir(BTN_HOME_PIN, GPIO_IN);
    // gpio_pull_up(BTN_HOME_PIN);

    // gpio_init(BTN_LOCK_PIN);
    // gpio_set_dir(BTN_LOCK_PIN, GPIO_IN);
    // gpio_pull_up(BTN_LOCK_PIN);

    zoom_init();
}

float wrap_angle(float angle) {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

// ---------------------------------------------------------------------------
// CUBE RUN
// ---------------------------------------------------------------------------
void cube_run(void) {
    float angle_x = HOME_ANGLE_X;
    float angle_y = HOME_ANGLE_Y;
    float angle_z = HOME_ANGLE_Z;

    absolute_time_t last_time = get_absolute_time();

    while (1) {

        // ---- BUTTONS ----
        if (btn_lock_flag) {
                if (s_locked) {
                    s_locked_angle_x = angle_x;
                    s_locked_angle_y = angle_y;
                }

                btn_lock_flag = false; 
            }

         // ---- IMU ----
        if (!s_locked) {
            IMUReading data = imu_read_accel(s_i2c, &s_cal, 10);

            absolute_time_t now = get_absolute_time();
            float dt = absolute_time_diff_us(last_time, now) / 1000000.0f;
            last_time = now;

            
            // --- ACCEL ANGLES ---
            float raw_accel_x = atan2f(data.ay, data.az) * 57.2958f;
            float raw_accel_y = atan2f(-data.ax, sqrtf(data.ay * data.ay + data.az * data.az)) * 57.2958f;

            // ---- HOME ----
            if (btn_home_flag) {
                offset_x = raw_accel_x - HOME_ANGLE_X;
                offset_y = raw_accel_y - HOME_ANGLE_Y;

                angle_x = HOME_ANGLE_X;
                angle_y = HOME_ANGLE_Y;
                angle_z = HOME_ANGLE_Z;

                btn_home_flag = false; 
            }
            
            float accel_angle_x = wrap_angle(raw_accel_x - offset_x);
            float accel_angle_y = wrap_angle(raw_accel_y - offset_y);

            // --- GYRO (deg/sec assumed) ---
            float gx = data.gx;
            float gy = data.gy;
            float gz = data.gz;

            // --- COMPLEMENTARY FILTER ---
            angle_x = ALPHA * (angle_x + gx * dt) + (1.0f - ALPHA) * accel_angle_x;
            angle_y = ALPHA * (angle_y + gy * dt) + (1.0f - ALPHA) * accel_angle_y;

            // Yaw (still drifts)
            angle_z += gz * dt;

            printf("angle_x: %.2f angle_y: %.2f angle_z: %.2f\n", angle_x, angle_y, angle_z);
        } else {
            angle_x = s_locked_angle_x;
            angle_y = s_locked_angle_y;
        }
        // ---- TRANSFORM ----
        Point3D projected[MAX_VERTICES];

        float dynamic_fov = zoom_get_fov();

        // angle_x = ((int)angle_x + 2) % 360;
        // angle_y = ((int)angle_y + 1) % 360;
        // angle_z = ((int)angle_z + 3) % 360;

        for (int i = 0; i < s_model.num_vertices; i++) {
            Point3D r = s_model.vertices[i];
            r = point3d_rotate_x(r, angle_x);
            r = point3d_rotate_y(r, -angle_z);
            r = point3d_rotate_z(r, angle_y);
            
            projected[i] = point3d_project(r, SCREEN_WIDTH, SCREEN_HEIGHT, dynamic_fov, PROJ_DISTANCE);
        }

        // ---- RENDER ----
        clear_screen();
        for (int i = 0; i < s_model.num_edges; i++) {
            Point3D *a = &projected[s_model.edges[i][0]];
            Point3D *b = &projected[s_model.edges[i][1]];
            
            draw_line((int)a->x, (int)a->y, (int)b->x, (int)b->y, DRAW_COLOR);
        }

        // ---- WAIT FOR FRAME FINISH ----
        address_pointer = back_buf;
        while (!vsync_flag) tight_loop_contents();
        vsync_flag = false;
        
        // ---- SWAP BUFFERS ----
        uint8_t* temp = front_buf;
        front_buf = back_buf;
        back_buf = temp;
        
        // while (!vsync_flag) tight_loop_contents();
        // vsync_flag = false;
    }
}

void cube_irq_init() {
    gpio_init(BTN_HOME_PIN);
    gpio_init(BTN_LOCK_PIN);
    gpio_set_dir(BTN_HOME_PIN, false);
    gpio_set_dir(BTN_LOCK_PIN, false);
    gpio_pull_up(BTN_HOME_PIN);
    gpio_pull_up(BTN_LOCK_PIN);

    gpio_set_irq_enabled_with_callback(BTN_HOME_PIN, GPIO_IRQ_EDGE_FALL, true, &cube_isr_handler);
    gpio_set_irq_enabled_with_callback(BTN_LOCK_PIN, GPIO_IRQ_EDGE_FALL, true, &cube_isr_handler);
    gpio_set_irq_enabled(BTN_LOCK_PIN, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_LOCK_PIN, GPIO_IRQ_EDGE_FALL, true);
    irq_set_enabled(IO_IRQ_BANK0, true);
}

void cube_isr_handler(int gpio, int events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (gpio == BTN_HOME_PIN) {
        if (current_time - btn_home_last_time > DEBOUNCE_MS) {
            btn_home_flag = true;
            btn_home_last_time = current_time;
        }
    }
    else if (gpio == BTN_LOCK_PIN) {
        if (current_time - btn_lock_last_time > DEBOUNCE_MS) {
            btn_lock_flag = true;
            s_locked = !s_locked; // toggle
            btn_lock_last_time = current_time;
        }
    }
}
