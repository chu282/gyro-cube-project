#include "imu.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

// ---------------------------------------------------------------------------
// MPU6050 REGISTER MAP
// ---------------------------------------------------------------------------
#define MPU_PWR_MGMT_1      0x6B
#define MPU_SMPLRT_DIV      0x19
#define MPU_CONFIG_REG      0x1A   // Named MPU_CONFIG_REG to avoid SDK macro clash
#define MPU_GYRO_CONFIG     0x1B
#define MPU_ACCEL_CONFIG    0x1C
#define MPU_ACCEL_XOUT_H    0x3B

// ---------------------------------------------------------------------------
// LOW-LEVEL I2C HELPERS (static — not exposed outside this file)
// ---------------------------------------------------------------------------
static void mpu_write(i2c_inst_t *i2c, uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    i2c_write_blocking(i2c, MPU6050_ADDR, buf, 2, false);
}

static void mpu_read(i2c_inst_t *i2c, uint8_t reg, uint8_t *buf, uint8_t len) {
    i2c_write_blocking(i2c, MPU6050_ADDR, &reg, 1, true);   // true = keep bus
    i2c_read_blocking (i2c, MPU6050_ADDR, buf, len, false);
}

// Read raw 16-bit accel values for all three axes (6 bytes starting at 0x3B)
static void mpu_read_accel_raw(i2c_inst_t *i2c, int16_t out[3]) {
    uint8_t buf[6];
    mpu_read(i2c, MPU_ACCEL_XOUT_H, buf, 6);
    out[0] = (int16_t)((buf[0] << 8) | buf[1]);   // X
    out[1] = (int16_t)((buf[2] << 8) | buf[3]);   // Y
    out[2] = (int16_t)((buf[4] << 8) | buf[5]);   // Z
}

// ---------------------------------------------------------------------------
// PUBLIC API
// ---------------------------------------------------------------------------

void imu_init(i2c_inst_t *i2c, uint sda_pin, uint scl_pin, uint baud) {
    // Bring up the I2C peripheral and configure the GPIO pins
    i2c_init(i2c, baud);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);

    sleep_ms(500);   // let the MPU6050 power on

    // Wake from sleep, use PLL with X-axis gyro reference (more stable than
    // the internal 8 MHz oscillator)
    mpu_write(i2c, MPU_PWR_MGMT_1,   0x01);

    // Sample rate divider: SMPLRT_DIV=7 → 1 kHz / (1+7) = 125 Hz sample rate
    mpu_write(i2c, MPU_SMPLRT_DIV,   0x07);

    // DLPF: bandwidth ~44 Hz accel / 42 Hz gyro — reduces noise without much lag
    mpu_write(i2c, MPU_CONFIG_REG,   0x03);

    // Gyro full-scale ±250 °/s (default, lowest noise)
    mpu_write(i2c, MPU_GYRO_CONFIG,  0x00);

    // Accel full-scale ±2 g (default) → 16384 LSB/g
    // ACCEL_SCALE=256 in cube.c maps ±16384 raw → ±64° rotation, which gives a
    // comfortable tilt-to-angle feel.
    mpu_write(i2c, MPU_ACCEL_CONFIG, 0x00);
}

IMUReading imu_calibrate(i2c_inst_t *i2c, int n_samples) {
    int32_t sum[3] = {0, 0, 0};
    int16_t raw[3];

    for (int i = 0; i < n_samples; i++) {
        mpu_read_accel_raw(i2c, raw);
        sum[0] += raw[0];
        sum[1] += raw[1];
        sum[2] += raw[2];
        sleep_ms(2);   // ~500 Hz — spread samples to catch low-freq vibration
    }

    return (IMUReading){
        .ax = sum[0] / (float)n_samples,
        .ay = sum[1] / (float)n_samples,
        .az = sum[2] / (float)n_samples,
    };
}

IMUReading imu_read_accel(i2c_inst_t *i2c, const IMUReading *cal, int n_samples) {
    int32_t sum[3] = {0, 0, 0};
    int16_t raw[3];

    for (int i = 0; i < n_samples; i++) {
        mpu_read_accel_raw(i2c, raw);
        sum[0] += raw[0];
        sum[1] += raw[1];
        sum[2] += raw[2];
    }

    // Average then subtract the at-rest calibration baseline
    return (IMUReading){
        .ax = (sum[0] / (float)n_samples) - cal->ax,
        .ay = (sum[1] / (float)n_samples) - cal->ay,
        .az = (sum[2] / (float)n_samples) - cal->az,
    };
}
