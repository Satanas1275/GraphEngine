#ifndef PLAYER_H
#define PLAYER_H

#include <gint/keyboard.h>
#include "math3d.h"
#include "physics.h"
#include "../scenes/map.h"

// ─── État du joueur ──────────────────────────────────────────────────────────

typedef struct {
    Vec3  feet;       // position des pieds (physique)
    float yaw;        // rotation horizontale (ay)
    float pitch;      // rotation verticale   (ax)
    float vel_y;      // vélocité verticale
    int   on_ground;  // 1 si au sol ou sur un cube
} Player;

// ─── Initialisation ──────────────────────────────────────────────────────────

static inline Player player_init(void) {
    return (Player){
        .feet      = PLAYER_START,
        .yaw       = 0.0f,
        .pitch     = 0.0f,
        .vel_y     = 0.0f,
        .on_ground = 0,
    };
}

// Position des yeux (pour le rendu)
static inline Vec3 player_eye(const Player *p) {
    Vec3 eye = p->feet;
    eye.y -= EYE_OFFSET;
    return eye;
}

// ─── Update (appelé à chaque substep) ───────────────────────────────────────

#define ROT_SPEED  0.01f

static inline void player_update(Player *p) {
    // ── Rotation ──
    if (keydown(KEY_LEFT))  p->yaw   += ROT_SPEED;
    if (keydown(KEY_RIGHT)) p->yaw   -= ROT_SPEED;
    if (keydown(KEY_UP))    p->pitch += ROT_SPEED;
    if (keydown(KEY_DOWN))  p->pitch -= ROT_SPEED;

    if (p->pitch >  1.4f) p->pitch =  1.4f;
    if (p->pitch < -1.4f) p->pitch = -1.4f;

    // ── Déplacement XZ ──
    float sinY = sinf(p->yaw), cosY = cosf(p->yaw);
    float dx = 0.0f, dz = 0.0f;

    if (keydown(KEY_8)) { dx -= sinY*MOVE_SPEED; dz += cosY*MOVE_SPEED; }
    if (keydown(KEY_2)) { dx += sinY*MOVE_SPEED; dz -= cosY*MOVE_SPEED; }
    if (keydown(KEY_4)) { dx -= cosY*MOVE_SPEED; dz -= sinY*MOVE_SPEED; }
    if (keydown(KEY_6)) { dx += cosY*MOVE_SPEED; dz += sinY*MOVE_SPEED; }

    if (dx != 0.0f || dz != 0.0f)
        physics_move_xz(&p->feet, dx, dz);

    // ── Saut ──
    if ((keydown(KEY_7) || keydown(KEY_9)) && p->on_ground) {
        p->vel_y    = -JUMP_FORCE;
        p->on_ground = 0;
    }

    // ── Gravité + déplacement Y ──
    if (!p->on_ground) {
        p->vel_y  += GRAVITY;
        p->feet.y += p->vel_y;
    }

    // ── Collision Y cubes ──
    int on_cube = physics_resolve_y(&p->feet, &p->vel_y);

    // ── Sol ──
    if (p->feet.y >= GROUND_Y) {
        p->feet.y    = GROUND_Y;
        p->vel_y     = 0.0f;
        p->on_ground = 1;
    } else if (on_cube) {
        p->on_ground = 1;
    } else {
        p->on_ground = 0;
    }

    // ── Vol libre debug (KEY_3 = descendre) ──
    if (keydown(KEY_3)) {
        p->feet.y   += MOVE_SPEED;
        p->vel_y     = 0.0f;
        p->on_ground = 0;
    }
}

#endif // PLAYER_H
