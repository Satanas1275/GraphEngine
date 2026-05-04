#ifndef PHYSICS_H
#define PHYSICS_H

#include "math3d.h"
#include "../scenes/map.h"

// ─── AABB ────────────────────────────────────────────────────────────────────

typedef struct {
    float min_x, max_x;
    float min_y, max_y;
    float min_z, max_z;
} AABB;

static inline AABB aabb_from_scene_object(int i) {
    float s  = SCENE_OBJECTS[i].scale;
    float cx = SCENE_OBJECTS[i].pos.x;
    float cy = SCENE_OBJECTS[i].pos.y;
    float cz = SCENE_OBJECTS[i].pos.z;
    return (AABB){ cx-s, cx+s, cy-s, cy+s, cz-s, cz+s };
}

// ─── Constantes joueur ───────────────────────────────────────────────────────

#define PLAYER_RADIUS  0.30f
#define PLAYER_HEIGHT  1.8f
#define EYE_OFFSET     0.8f

// ─── Collision XZ ────────────────────────────────────────────────────────────
//
// Résout la pénétration XZ contre un AABB.
// Ignorée si le joueur est au-dessus ou en-dessous du cube.
// Y augmente vers le bas : min_y = dessus du cube, max_y = dessous du cube.

static inline void collide_xz_aabb(Vec3 *p, float feet_y, AABB box, float r) {
    float head_y = feet_y - PLAYER_HEIGHT;

    // Pas d'intersection en Y → pas de collision latérale
    // Joueur au-dessus : feet_y <= box.min_y (pieds au-dessus du dessus)
    // Joueur en-dessous : head_y >= box.max_y (tête en-dessous du dessous)
    if (feet_y <= box.min_y || head_y >= box.max_y) return;

    // Chevauchement XZ ?
    if (p->x < box.min_x - r || p->x > box.max_x + r) return;
    if (p->z < box.min_z - r || p->z > box.max_z + r) return;

    // Pénétration minimale
    float dx_left  = p->x - (box.min_x - r);
    float dx_right = (box.max_x + r) - p->x;
    float dz_front = p->z - (box.min_z - r);
    float dz_back  = (box.max_z + r) - p->z;

    float px = (dx_left < dx_right) ? -dx_left : dx_right;
    float pz = (dz_front < dz_back) ? -dz_front : dz_back;

    if (fabsf(px) < fabsf(pz)) p->x += px;
    else                        p->z += pz;
}

// ─── Collision Y ─────────────────────────────────────────────────────────────
//
// Y augmente vers le bas (gravité positive).
// box.min_y = dessus du cube, box.max_y = dessous du cube.
// Retourne 1 si posé sur un cube.

static inline int collide_y_aabb(Vec3 *feet, float *vel_y, AABB box, float r) {
    if (feet->x < box.min_x - r || feet->x > box.max_x + r) return 0;
    if (feet->z < box.min_z - r || feet->z > box.max_z + r) return 0;

    float prev_y = feet->y - *vel_y;
    float head_y = feet->y - PLAYER_HEIGHT;
    float prev_head = prev_y - PLAYER_HEIGHT;

    if (*vel_y >= 0.0f) {
        // Chute (Y croît) → atterrissage sur box.min_y (dessus)
        if (prev_y <= box.min_y && feet->y >= box.min_y) {
            feet->y = box.min_y;
            *vel_y  = 0.0f;
            return 1;
        }
    } else {
        // Montée (Y décroît) → tête cogne box.max_y (dessous)
        if (prev_head >= box.max_y && head_y <= box.max_y) {
            feet->y = box.max_y + PLAYER_HEIGHT;
            *vel_y  = 0.0f;
        }
    }
    return 0;
}

// ─── Mouvement XZ avec collisions ────────────────────────────────────────────

static inline void physics_move_xz(Vec3 *cam, float dx, float dz) {
    Vec3 next = *cam;
    next.x += dx;
    next.z += dz;

    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < SCENE_OBJECT_COUNT; i++) {
            if (SCENE_OBJECTS[i].type != OBJ_CUBE) continue;
            AABB box = aabb_from_scene_object(i);
            collide_xz_aabb(&next, cam->y, box, PLAYER_RADIUS);
        }
    }

    if (next.x < WALL_MIN + PLAYER_RADIUS) next.x = WALL_MIN + PLAYER_RADIUS;
    if (next.x > WALL_MAX - PLAYER_RADIUS) next.x = WALL_MAX - PLAYER_RADIUS;
    if (next.z < WALL_MIN + PLAYER_RADIUS) next.z = WALL_MIN + PLAYER_RADIUS;
    if (next.z > WALL_MAX - PLAYER_RADIUS) next.z = WALL_MAX - PLAYER_RADIUS;

    cam->x = next.x;
    cam->z = next.z;
}

// Retourne 1 si posé sur un cube
static inline int physics_resolve_y(Vec3 *cam, float *vel_y) {
    for (int i = 0; i < SCENE_OBJECT_COUNT; i++) {
        if (SCENE_OBJECTS[i].type != OBJ_CUBE) continue;
        AABB box = aabb_from_scene_object(i);
        if (collide_y_aabb(cam, vel_y, box, PLAYER_RADIUS)) return 1;
    }
    return 0;
}

#endif // PHYSICS_H
