#define WASM_EXPORT __attribute__((visibility("default")))
#include <stdint.h>

/* -- BEGIN WASM -- */
extern void print(float f);
extern float sqrtf(float f);
extern float cosf(float f);
extern float sinf(float f);
extern unsigned char __heap_base;
#define PAGE_SIZE (1 << 16)
/* -- END WASM -- */

/* -- BEGIN MATH -- */
typedef struct { float x, y; } Vec2;
typedef struct { float x, y, z; } Vec3;

typedef union {
  Vec3 xyz;
  struct { float x, y, z, w; };
  float nums[4];
} Vec4;

static Vec3 sub3(Vec3 a, Vec3 b) { return (Vec3) { a.x - b.x,
                                                   a.y - b.y,
                                                   a.z - b.z, }; }
static Vec3 mul3_f(Vec3 a, float f) { return (Vec3) { a.x * f,
                                                      a.y * f,
                                                      a.z * f, }; }
static Vec3 div3_f(Vec3 a, float f) { return (Vec3) { a.x / f,
                                                      a.y / f,
                                                      a.z / f, }; }
static float dot3(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static float mag3(Vec3 v) { return sqrtf(dot3(v, v)); }
static Vec3 norm3(Vec3 v) { return div3_f(v, mag3(v)); }
static Vec3 cross3(Vec3 a, Vec3 b) {
  return (Vec3){(a.y * b.z) - (a.z * b.y),
                (a.z * b.x) - (a.x * b.z),
                (a.x * b.y) - (a.y * b.x)};
}

typedef struct { float nums[4][4]; } Mat4;

static Mat4 look_at4x4(Vec3 eye, Vec3 focus, Vec3 up) {
  Vec3 eye_dir = sub3(focus, eye);
  Vec3 R2 = norm3(eye_dir);

  Vec3 R0 = norm3(cross3(up, R2));
  Vec3 R1 = cross3(R2, R0);

  Vec3 neg_eye = mul3_f(eye, -1.0f);

  float D0 = dot3(R0, neg_eye);
  float D1 = dot3(R1, neg_eye);
  float D2 = dot3(R2, neg_eye);

  return (Mat4) {{
    { R0.x, R1.x, R2.x, 0.0f },
    { R0.y, R1.y, R2.y, 0.0f },
    { R0.z, R1.z, R2.z, 0.0f },
    {   D0,   D1,   D2, 1.0f }
  }};
}

static Vec4 mul4x44(Mat4 m, Vec4 v) {
  Vec4 res;
  for(int x = 0; x < 4; ++x) {
    float sum = 0;
    for(int y = 0; y < 4; ++y)
      sum += m.nums[y][x] * v.nums[y];

    res.nums[x] = sum;
  }
  return res;
}
/* -- END MATH -- */

/* -- BEGIN RENDER -- */
typedef struct { uint8_t r, g, b, a; } Color;

static struct {
  int width, height;
  uint8_t *pixels;
  Mat4 mvp;
} rendr;

static Vec2 world_to_screen(Vec3 world) {
  Vec4 p;
  p.xyz = world;
  p.  w = 1;
  p = mul4x44(rendr.mvp, p);
  return (Vec2) {
    rendr. width * (p.x + 0.5),
    rendr.height * (p.y + 0.5),
  };
}
/* -- END RENDER -- */

WASM_EXPORT uint8_t *init(int width, int height) {
  rendr.width = width;
  rendr.height = height;
  rendr.pixels = &__heap_base;
  int mem_needed = (width * height * 4)/PAGE_SIZE;
  int delta = mem_needed - __builtin_wasm_memory_size(0) + 2;

  if (delta > 0) __builtin_wasm_memory_grow(0, delta);

  return rendr.pixels;
}

static void write_pixel(int x, int y, Color c) {
  if (x < 0 || x >= rendr. width) return;
  if (y < 0 || y >= rendr.height) return;
  rendr.pixels[((y * rendr.width) + x)*4 + 0] = c.r;
  rendr.pixels[((y * rendr.width) + x)*4 + 1] = c.g;
  rendr.pixels[((y * rendr.width) + x)*4 + 2] = c.b;
  rendr.pixels[((y * rendr.width) + x)*4 + 3] = c.a;
}

#if 0
static void fill_rect(int px, int py) {
  for (int x = px-5; x < px+5; x++)
    for (int y = py-5; y < py+5; y++)
      write_pixel(x, y, (Color) { 255, 255, 9, 255 });
}
#endif

#define abs(a) (((a) < 0) ? -(a) : (a))
static void plot_line(Vec2 p0, Vec2 p1, Color c) {
  int x0 = p0.x, y0 = p0.y;
  int x1 = p1.x, y1 = p1.y;

  int dx = abs(x1 - x0);
  int sx = (x0 < x1) ? 1 : -1;
  int dy = -abs(y1 - y0);
  int sy = (y0 < y1) ? 1 : -1;

  int e2;
  int error = dx + dy;

  while (1) {
    write_pixel(x0, y0, c);
    if (x0 == x1 && y0 == y1) break;
    e2 = 2 * error;
    if (e2 >= dy) {
      if (x0 == x1) break;
      error += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      if (y0 == y1) break;
      error += dx;
      y0 += sy;
    }
  }
}

WASM_EXPORT void draw(float dt) {
  __builtin_memset(rendr.pixels, 235, rendr.width * rendr.height * 4);

  rendr.mvp = look_at4x4(
    (Vec3) { sinf(dt), cosf(dt), 1.0f + sinf(dt*15) * 0.1f },
    (Vec3) { 0, 0, 0 },
    (Vec3) { 0, 0, 1 }
  );

  for (float x = -0.2f; x < 0.2f; x += 0.02f)
    plot_line(
      world_to_screen((Vec3){ x, -0.15f, 0.0f }),
      world_to_screen((Vec3){ x,  0.15f, 0.0f }),
      (Color) { 135, 155, 255, 255 }
    );
  plot_line(
    world_to_screen((Vec3){ -0.25f, -0.11f, 0.0f }),
    world_to_screen((Vec3){  0.22f, -0.11f, 0.0f }),
    (Color) { 255, 155, 155, 255 }
  );

  // for (int x = 0; x < rendr.width; x++)
  //   for (int y = 0; y < rendr.height; y++) {
  //     float r = fmod(((float) x + dt) / (float) rendr.width, 0.5);
  //     float g = fmod(((float) y + dt) / (float) rendr.height, 0.5);
  //     rendr.pixels[((y * rendr.width) + x)*4 + 0] = 255 * r;
  //     rendr.pixels[((y * rendr.width) + x)*4 + 1] = 255 * g;
  //     rendr.pixels[((y * rendr.width) + x)*4 + 2] =   9;
  //     rendr.pixels[((y * rendr.width) + x)*4 + 3] = 255; // alpha
  //   }
}
