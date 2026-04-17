#include "math.h"
#include <stdlib.h>
#include <string.h>


// ---------------------------------------------------------------------------
// ROTATION FUNCTIONS
// ---------------------------------------------------------------------------

Point3D point3d_rotate_x(Point3D p, float deg) {
    float rad  = deg * (float)M_PI / 180.0f;
    float cosa = cosf(rad);
    float sina = sinf(rad);
    return (Point3D){
        .x = p.x,
        .y = p.y * cosa - p.z * sina,
        .z = p.y * sina + p.z * cosa
    };
}

Point3D point3d_rotate_y(Point3D p, float deg) {
    float rad  = deg * (float)M_PI / 180.0f;
    float cosa = cosf(rad);
    float sina = sinf(rad);
    return (Point3D){
        .x = p.z * sina + p.x * cosa,
        .y = p.y,
        .z = p.z * cosa - p.x * sina
    };
}

Point3D point3d_rotate_z(Point3D p, float deg) {
    float rad  = deg * (float)M_PI / 180.0f;
    float cosa = cosf(rad);
    float sina = sinf(rad);
    return (Point3D){
        .x = p.x * cosa - p.y * sina,
        .y = p.x * sina + p.y * cosa,
        .z = p.z
    };
}

// ---------------------------------------------------------------------------
// Perspective Projection
// ---------------------------------------------------------------------------
Point3D point3d_project(Point3D p, int width, int height,
                        float fov, float viewer_distance) {
    float factor = fov / (viewer_distance + p.z);
    return (Point3D){
        .x =  p.x * factor + (float)width  / 2.0f,
        .y = -p.y * factor + (float)height / 2.0f,
        .z = p.z
    };
}

// ---------------------------------------------------------------------------
// Bresenham's line algorithm: Draw's a straight line between 2 points
// ---------------------------------------------------------------------------
void draw_line(int x0, int y0, int x1, int y1, char color) {
    int dx  =  abs(x1 - x0);
    int dy  =  abs(y1 - y0);
    int sx  = (x0 < x1) ? 1 : -1;
    int sy  = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

// ---------------------------------------------------------------------------
//Clear Screen
// ---------------------------------------------------------------------------
void clear_screen(void) {
    memset(vga_data, 0, TXCOUNT);
}
