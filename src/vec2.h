#ifndef VEC2_HEADER
#define VEC2_HEADER

typedef struct {
    float x;
    float y;
} Vec2;

typedef struct {
   Vec2 top_left;
   Vec2 bottom_right;
} Rect;

static void Vec2_add(Vec2 *out, Vec2 a, Vec2 b) {
    out->x = a.x + b.x;
    out->y = a.y + b.y;
}

static void Vec2_multiply_scalar(Vec2 *out, Vec2 vec, float scalar) {
    out->x = vec.x * scalar;
    out->y = vec.y * scalar;
}

static void Vec2_set(Vec2 *out, float x, float y) {
    out->x = x;
    out->y = y;
}

#endif
