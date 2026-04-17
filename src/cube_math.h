#pragma once

#include <math.h>
#include <string.h>

// Screen dimensions — must match the values in vga.c
#define SCREEN_WIDTH  1024
#define SCREEN_HEIGHT 600

// Two pixels are packed per byte in vga_data[], so the buffer is half the
// total pixel count.
#define TXCOUNT (SCREEN_WIDTH * SCREEN_HEIGHT / 2)

// Suppress redefinition warnings if <math.h> doesn't supply M_PI
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------------------------------------------------------------------------
// Point3D
// Used for both 3D world-space vertices and 2D projected screen coordinates.
// When used as a projected point, x and y are screen pixels (floats) and z
// is the original depth (kept so callers can inspect it if needed).
// ---------------------------------------------------------------------------
typedef struct {
    float x, y, z;
} Point3D;

// Rotate p around the X axis by deg degrees (positive = right-hand rule).
Point3D point3d_rotate_x(Point3D p, float deg);

// Rotate p around the Y axis by deg degrees.
Point3D point3d_rotate_y(Point3D p, float deg);

// Rotate p around the Z axis by deg degrees.
Point3D point3d_rotate_z(Point3D p, float deg);

// Project a 3D point onto a 2D screen using a perspective divide.
//   fov            : field-of-view scale factor (pixels; larger = more zoom)
//   viewer_distance: distance from the viewer to the origin along Z
// The returned Point3D has x,y in screen-pixel space and z unchanged.
Point3D point3d_project(Point3D p, int width, int height,
                        float fov, float viewer_distance);

// Draw a line between two screen-space integer coordinates using Bresenham's
// algorithm.  color is a 4-bit VGA palette index (see color #defines in
// vga.c).
// NOTE: drawPixel() is declared extern below; it is defined in vga.c which
//       is #included directly by main.c and resolved at link time.
void draw_line(int x0, int y0, int x1, int y1, char color);

// Zero every pixel in the VGA framebuffer.
// Must be called at the start of each frame because drawPixel() ORs color
// bits rather than replacing them — without clearing, ghost lines accumulate.
// NOTE: vga_data[] is declared extern below; defined in vga.c / main.c.
void clear_screen(void);

// ---------------------------------------------------------------------------
// Forward declarations for symbols provided by vga.c
// vga.c is #included (not compiled separately) by main.c, so these resolve
// at link time from main.c's translation unit.
// ---------------------------------------------------------------------------
extern void drawPixel(int x, int y, char color);
extern char vga_data[];
