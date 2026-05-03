#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/timer.h>
#include <gint/clock.h>
#include <math.h>
#include <stdio.h>

#include "scenes/map.h"

#define SCREEN_W 128
#define SCREEN_H 64
#define SIZE 10
#define ROT_SPEED 0.01f

// Hauteur des yeux pour point de vue First-Person (rendu)
// 0.8f est une valeur correcte au vu de l'échelle du monde (cube demi-taille = 1)
#define EYE_HEIGHT 0.8f

// Adjustable target FPS
#define TARGET_FPS 30
// Physics substeps per frame; tweak with FPS if desired
#define SUBSTEPS 5

// Radius for the "player" (camera) used for collisions
#define PLAYER_RADIUS 0.30f

// Cube template vertices (half-size = 1)
static const Vec3 cube[8] = {
    {-1,-1,-1}, {1,-1,-1}, {1,1,-1}, {-1,1,-1},
    {-1,-1,1},  {1,-1,1},  {1,1,1},  {-1,1,1}
};

static const int edges[12][2] = {
    {0,1},{1,2},{2,3},{3,0},
    {4,5},{5,6},{6,7},{7,4},
    {0,4},{1,5},{2,6},{3,7}
};

// Forward declarations
void project(Vec3 v, Vec3 cam, float ax, float ay, int *sx, int *sy);

// Draw a cube at a world position with scale
void draw_cube(Vec3 pos, float scale, Vec3 cam, float ax, float ay) {
    for(int i=0; i<12; i++) {
        Vec3 v1 = { cube[edges[i][0]].x * scale + pos.x, cube[edges[i][0]].y * scale + pos.y, cube[edges[i][0]].z * scale + pos.z };
        Vec3 v2 = { cube[edges[i][1]].x * scale + pos.x, cube[edges[i][1]].y * scale + pos.y, cube[edges[i][1]].z * scale + pos.z };
        int x1,y1,x2,y2;
        project(v1, cam, ax, ay, &x1,&y1);
        project(v2, cam, ax, ay, &x2,&y2);
        dline(x1,y1,x2,y2,C_BLACK);
    }
}

// Rotate point by angles around origin
void rotate(Vec3 *p, float ax, float ay) {
    float x=p->x, y=p->y, z=p->z;
    float x1 = x*cosf(ay)-z*sinf(ay);
    float z1 = x*sinf(ay)+z*cosf(ay);
    float y1 = y*cosf(ax)-z1*sinf(ax);
    float z2 = y*sinf(ax)+z1*cosf(ax);
    p->x = x1; p->y = y1; p->z = z2;
}

// Project vertex relative to camera
void project(Vec3 v, Vec3 cam, float ax, float ay, int *sx, int *sy) {
    Vec3 p = { v.x - cam.x, v.y - cam.y, v.z - cam.z };
    rotate(&p, -ax, -ay);
    float scale = 40.0f / (p.z + 5.0f);
    *sx = (int)(p.x * scale * SIZE + SCREEN_W/2);
    *sy = (int)(p.y * scale * SIZE + SCREEN_H/2);
}

// Clamp camera within world walls, accounting for radius
static inline void clamp_world(Vec3 *cam) {
    if (cam->x < WALL_MIN + PLAYER_RADIUS) cam->x = WALL_MIN + PLAYER_RADIUS;
    if (cam->x > WALL_MAX - PLAYER_RADIUS) cam->x = WALL_MAX - PLAYER_RADIUS;
    if (cam->z < WALL_MIN + PLAYER_RADIUS) cam->z = WALL_MIN + PLAYER_RADIUS;
    if (cam->z > WALL_MAX - PLAYER_RADIUS) cam->z = WALL_MAX - PLAYER_RADIUS;
}

// Resolve Y collision with one AABB, returns 1 if landed on top
static inline int resolve_y_against_box(Vec3 *cam, float *vel_y, float min_x, float max_x, float min_y, float max_y, float min_z, float max_z, float r) {
    if (cam->x < min_x - r || cam->x > max_x + r) return 0;
    if (cam->z < min_z - r || cam->z > max_z + r) return 0;
    if (cam->y >= min_y && cam->y <= max_y) {
        if (*vel_y > 0) {
            cam->y = max_y;
            *vel_y = 0;
            return 1;
        } else if (*vel_y < 0) {
            cam->y = min_y;
            *vel_y = 0;
            return 0;
        } else {
            float dist_to_min = cam->y - min_y;
            float dist_to_max = max_y - cam->y;
            if (dist_to_min < dist_to_max) cam->y = min_y;
            else cam->y = max_y;
            return (cam->y == max_y);
        }
    }
    return 0;
}

// Resolve Y collisions with all scene cubes, returns 1 if landed on a cube
static inline int resolve_y_collisions(Vec3 *cam, float *vel_y) {
    const float r = PLAYER_RADIUS;
    for (int i = 0; i < SCENE_OBJECT_COUNT; i++) {
        if (SCENE_OBJECTS[i].type != OBJ_CUBE) continue;
        float s = SCENE_OBJECTS[i].scale;
        float cx = SCENE_OBJECTS[i].pos.x;
        float cy = SCENE_OBJECTS[i].pos.y;
        float cz = SCENE_OBJECTS[i].pos.z;
        float min_x = cx - s, max_x = cx + s;
        float min_y = cy - s, max_y = cy + s;
        float min_z = cz - s, max_z = cz + s;
        if (resolve_y_against_box(cam, vel_y, min_x, max_x, min_y, max_y, min_z, max_z, r)) return 1;
    }
    return 0;
}

// Resolve minimal penetration on XZ against one AABB expanded by r, if overlapping in Y
static inline void resolve_against_box(Vec3 *p, float y, float min_x, float max_x, float min_y, float max_y, float min_z, float max_z, float r) {
    if (y < min_y - r || y > max_y + r) return;

    int inside_x = (p->x >= (min_x - r) && p->x <= (max_x + r));
    int inside_z = (p->z >= (min_z - r) && p->z <= (max_z + r));
    if (!(inside_x && inside_z)) return;

    float push_left  = (min_x - r) - p->x;
    float push_right = p->x - (max_x + r);
    float fix_x = (fabsf(push_left) < fabsf(push_right)) ? push_left : -push_right;

    float push_front = (min_z - r) - p->z;
    float push_back  = p->z - (max_z + r);
    float fix_z = (fabsf(push_front) < fabsf(push_back)) ? push_front : -push_back;

    if (fabsf(fix_x) < fabsf(fix_z)) p->x += fix_x; else p->z += fix_z;
}

// Move on XZ plane with collisions against scene cubes (AABB) and walls
void move_with_collisions(Vec3 *cam, float dx, float dz) {
    const float r = PLAYER_RADIUS;

    Vec3 p = *cam;
    p.x += dx;
    p.z += dz;

    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < SCENE_OBJECT_COUNT; i++) {
            if (SCENE_OBJECTS[i].type != OBJ_CUBE) continue;
            float s = SCENE_OBJECTS[i].scale;
            float cx = SCENE_OBJECTS[i].pos.x;
            float cy = SCENE_OBJECTS[i].pos.y;
            float cz = SCENE_OBJECTS[i].pos.z;
            float min_x = cx - s, max_x = cx + s;
            float min_y = cy - s, max_y = cy + s;
            float min_z = cz - s, max_z = cz + s;
            resolve_against_box(&p, cam->y, min_x, max_x, min_y, max_y, min_z, max_z, r);
        }
    }

    cam->x = p.x;
    cam->z = p.z;

    clamp_world(cam);
}

int main(void) {
    Vec3 cam = PLAYER_START; // Corps (collisions/physique)
    float ax=0, ay=0;
    float velocity_y = 0.0f;
    int on_ground = 0;

    while(1) {
        dclear(C_WHITE);

        // Vue: point de vue first-person = corps - EYE_HEIGHT sur Y
        Vec3 cam_view = cam;
        cam_view.y -= EYE_HEIGHT;

        // Draw scene objects
        for (int i = 0; i < SCENE_OBJECT_COUNT; i++) {
            if (SCENE_OBJECTS[i].type == OBJ_CUBE) {
                draw_cube(SCENE_OBJECTS[i].pos, SCENE_OBJECTS[i].scale, cam_view, ax, ay);
            }
        }

        // Info
        char buf[32];
        sprintf(buf, "Y:%.1f", cam.y);
        dtext(2, 2, C_BLACK, buf);
        if(on_ground) dtext(2, 10, C_BLACK, "GROUND");

        dupdate();

        // Inputs
        clearevents();

        // Physics substeps
        for(int frame=0; frame<SUBSTEPS; frame++) {
            if(keydown(KEY_EXIT)) {
                return 1;
            }

            // Head rotation
            if(keydown(KEY_LEFT))  ay += ROT_SPEED;
            if(keydown(KEY_RIGHT)) ay -= ROT_SPEED;
            if(keydown(KEY_UP))    ax += ROT_SPEED;
            if(keydown(KEY_DOWN))  ax -= ROT_SPEED;

            // Compute intended planar motion (XZ) from keys
            float sinA = sinf(ay), cosA = cosf(ay);
            float dx = 0.0f, dz = 0.0f;

            // Forward/back
            if(keydown(KEY_8)) { dx -= sinA*MOVE_SPEED; dz += cosA*MOVE_SPEED; }
            if(keydown(KEY_2)) { dx += sinA*MOVE_SPEED; dz -= cosA*MOVE_SPEED; }

            // Strafe left/right
            if(keydown(KEY_4)) { dx -= cosA*MOVE_SPEED; dz -= sinA*MOVE_SPEED; }
            if(keydown(KEY_6)) { dx += cosA*MOVE_SPEED; dz += sinA*MOVE_SPEED; }

            // Apply XZ movement with collisions against cubes and world
            move_with_collisions(&cam, dx, dz);

            // Jump
            if((keydown(KEY_7) || keydown(KEY_9)) && on_ground) {
                velocity_y = -JUMP_FORCE;
                on_ground = 0;
            }

            // Gravity always
            velocity_y += GRAVITY;
            cam.y += velocity_y;

            // Cube Y collision (land on top or hit bottom)
            int landed_on_cube = resolve_y_collisions(&cam, &velocity_y);

            // Ground collision
            if(cam.y >= GROUND_Y) {
                cam.y = GROUND_Y;
                velocity_y = 0.0f;
                on_ground = 1;
            } else if (landed_on_cube) {
                on_ground = 1;
            } else {
                on_ground = 0;
            }

            // Free-fly down
            if(keydown(KEY_3)) {
                cam.y += MOVE_SPEED;
                velocity_y = 0.0f;
            }

            // Small busy-wait to avoid maxing out CPU
            for(volatile int i=0; i<20000; i++);
        }

        // Simple frame pacing to approximate TARGET_FPS
        int frame_ms = 1000 / TARGET_FPS;
        sleep_ms(frame_ms);
    }

    return 1;
}