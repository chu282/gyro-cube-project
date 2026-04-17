#pragma once

#include "math.h"   // Point3D

// ---------------------------------------------------------------------------
// MODEL LIMITS & PATH
// ---------------------------------------------------------------------------
#define MAX_VERTICES  64
#define MAX_EDGES     128
#define MODEL_PATH    "model.bin"

// ---------------------------------------------------------------------------
// SimpleModel
// Holds a wireframe model loaded from the SD card binary format:
//   [int num_vertices][int num_edges][Point3D * num_vertices][int[2] * num_edges]
// ---------------------------------------------------------------------------
typedef struct {
    int     num_vertices;
    int     num_edges;
    Point3D vertices[MAX_VERTICES];
    int     edges[MAX_EDGES][2];
} SimpleModel;

// Initialize I2C, the MPU6050, and the two GPIO buttons.
// Must be called once after init_vga() and before cube_run().
void cube_init(void);

// Enter the main simulation loop.  Reads the accelerometer each frame,
// maps the readings to rotation angles, and renders the wireframe cube.
// This function does not return.
void cube_run(void);
