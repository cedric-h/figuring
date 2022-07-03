#define WASM_EXPORT __attribute__((visibility("default")))
#include <stdint.h>

extern void print(float i);
extern unsigned char __heap_base;
#define PAGE_SIZE (1 << 16)

static struct {
  int width, height;
  uint8_t *pixels;
} rendr;

WASM_EXPORT uint8_t *init(int width, int height) {
  rendr.width = width;
  rendr.height = height;
  rendr.pixels = &__heap_base;
  int mem_needed = (width * height * 4)/PAGE_SIZE;
  int delta = mem_needed - __builtin_wasm_memory_size(0) + 2;

  if (delta > 0) __builtin_wasm_memory_grow(0, delta);

  return rendr.pixels;
}

typedef union {
  struct { float x, y, z, w };
  float nums[4];
} Vec4;
static Vec4 points[] = {
  { 0.50f, 0.50f, 0.50f, 1.0f },
  { 0.25f, 0.25f, 0.50f, 1.0f },
  { 0.75f, 0.75f, 0.50f, 1.0f },
};

typedef struct { float nums[4][4]; } Mat4;

typedef struct { float x, y, z; } Vec3;
static Vec3 sub3(Vec3 a, Vec3 b) { return (Vec3) { a.x - b.x,
                                                   a.y - b.y,
                                                   a.z - b.z, }; }
static Vec3 div3(Vec3 a, Vec3 b) { return (Vec3) { a.x / b.x,
                                                   a.y / b.y,
                                                   a.z / b.z, }; }
static float dot3(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static Vec3 cross3(Vec3 a, Vec3 b) {
  return (Vec3){(a.y * b.z) - (a.z * b.y),
                (a.z * b.x) - (a.x * b.z),
                (a.x * b.y) - (a.y * b.x)};
}

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

static void fill_rect(int px, int py) {
  for (int x = px-5; x < px+5; x++)
    for (int y = py-5; y < py+5; y++) {
      rendr.pixels[((y * rendr.width) + x)*4 + 0] = 255;
      rendr.pixels[((y * rendr.width) + x)*4 + 1] = 255;
      rendr.pixels[((y * rendr.width) + x)*4 + 2] =   9;
      rendr.pixels[((y * rendr.width) + x)*4 + 3] = 255; // alpha
    }
}

WASM_EXPORT void draw(float dt) {
  __builtin_memset(rendr.pixels, 128, rendr.width * rendr.height * 4);

  for (int i = 0; i < sizeof(points) / sizeof(points[0]); i++) {
    fill_rect(
      rendr.width * points[i].x,
      rendr.height * points[i].y
    );
  }

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
