#pragma once

#include "hardware/i2c.h"
#include <stdint.h>

// ---------------------------------------------------------------------------
// MPU6050 I2C address
// AD0 pin LOW  → 0x68  (most common default)
// AD0 pin HIGH → 0x69
// Change this if your board has AD0 tied high.
// ---------------------------------------------------------------------------
#define MPU6050_ADDR 0x68

// ---------------------------------------------------------------------------
// IMUReading
// Raw accelerometer values after averaging, with calibration removed.
// Units are raw MPU6050 ADC counts (±32768 full scale at ±2 g default range).
// Divide by ~182 to get g-force, or use the ACCEL_SCALE factor in cube.c to
// map directly to rotation angles.
// ---------------------------------------------------------------------------
typedef struct {
    float ax, ay, az;
} IMUReading;

// Initialize the MPU6050 and the I2C peripheral.
//   i2c     : i2c0 or i2c1
//   sda_pin : GPIO number for SDA
//   scl_pin : GPIO number for SCL
//   baud    : I2C clock rate in Hz (e.g. 400000 for fast-mode)
void imu_init(i2c_inst_t *i2c, uint sda_pin, uint scl_pin, uint baud);

// Collect n_samples readings at rest and return their mean as the calibration
// baseline.  The device must be stationary and flat during this call.
// Pass the result to imu_read_accel() to remove the at-rest bias.
IMUReading imu_calibrate(i2c_inst_t *i2c, int n_samples);

// Read the accelerometer n_samples times, average, then subtract cal.
// Returns calibrated accelerometer readings: near-zero when the device is in
// the same orientation as when imu_calibrate() was called.
IMUReading imu_read_accel(i2c_inst_t *i2c, const IMUReading *cal, int n_samples);
