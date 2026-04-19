void draw_pixel(int x, int y, char color);
void draw_rect(int x, int y, int w, int h, char color);
void draw_colorboard();
void init_vga();

// colors
#define BLACK        0   // 0000
#define RED          1   // 0001
#define BLUE         2   // 0010
#define MAGENTA      3   // 0011 (R+B)
#define DARK_GREEN   4   // 0100 (G_Lo)
#define DARK_YELLOW  5   // 0101 (R+G_Lo)
#define DARK_CYAN    6   // 0110 (B+G_Lo)
#define DARK_GRAY    7   // 0111 (R+B+G_Lo)
#define MED_GREEN    8   // 1000 (G_Hi)
#define ORANGE       9   // 1001 (R+G_Hi)
#define LIGHT_BLUE   10  // 1010 (B+G_Hi)
#define PINK         11  // 1011 (R+B+G_Hi)
#define BRIGHT_GREEN 12  // 1100 (G_Lo+G_Hi)
#define YELLOW       13  // 1101 (R+G_Lo+G_Hi)
#define CYAN         14  // 1110 (B+G_Lo+G_Hi)
#define WHITE        15  // 1111 (All Pins)