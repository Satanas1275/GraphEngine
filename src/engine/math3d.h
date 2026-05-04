#ifndef MATH3D_H
#define MATH3D_H

#include <math.h>

// ─── Vecteur 3D ───────────────────────────────────────────────────────────────

#ifndef VEC3_DEFINED
typedef struct { float x, y, z; } Vec3;
#define VEC3_DEFINED
#endif

static inline Vec3 vec3_add(Vec3 a, Vec3 b) {
    return (Vec3){ a.x+b.x, a.y+b.y, a.z+b.z };
}
static inline Vec3 vec3_sub(Vec3 a, Vec3 b) {
    return (Vec3){ a.x-b.x, a.y-b.y, a.z-b.z };
}
static inline Vec3 vec3_scale(Vec3 v, float s) {
    return (Vec3){ v.x*s, v.y*s, v.z*s };
}
static inline float vec3_dot(Vec3 a, Vec3 b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}
static inline Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return (Vec3){
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    };
}
static inline float vec3_len(Vec3 v) {
    return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
}
static inline Vec3 vec3_norm(Vec3 v) {
    float l = vec3_len(v);
    if (l < 0.0001f) return (Vec3){0,0,0};
    return vec3_scale(v, 1.0f/l);
}

// ─── Rotation + Projection ───────────────────────────────────────────────────

// Rotation d'un point autour de l'axe X (pitch) puis Y (yaw)
static inline Vec3 rotate_xy(Vec3 p, float ax, float ay) {
    // Yaw (rotation autour de Y)
    float x1 = p.x*cosf(ay) - p.z*sinf(ay);
    float z1 = p.x*sinf(ay) + p.z*cosf(ay);
    // Pitch (rotation autour de X)
    float y1 = p.y*cosf(ax) - z1*sinf(ax);
    float z2 = p.y*sinf(ax) + z1*cosf(ax);
    return (Vec3){ x1, y1, z2 };
}

// Projection perspective d'un point monde vers écran
// Retourne 0 si derrière la caméra (à ne pas afficher)
#define SCREEN_W 128
#define SCREEN_H 64
#define FOV_SCALE 40.0f
#define NEAR_PLANE 0.1f

static inline int project_point(Vec3 world, Vec3 cam_pos, float ax, float ay,
                                 int *sx, int *sy)
{
    Vec3 p = rotate_xy(vec3_sub(world, cam_pos), -ax, -ay);
    float dz = p.z + 5.0f;
    if (dz < NEAR_PLANE) return 0;
    float sc = FOV_SCALE / dz;
    *sx = (int)(p.x * sc * 10 + SCREEN_W/2);
    *sy = (int)(p.y * sc * 10 + SCREEN_H/2);
    return 1;
}

// Distance au carré entre deux Vec3 (évite sqrt)
static inline float vec3_dist2(Vec3 a, Vec3 b) {
    Vec3 d = vec3_sub(a, b);
    return vec3_dot(d, d);
}

#endif // MATH3D_H
