#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/timer.h>
#include <math.h>
#include <stdio.h>

#define SCREEN_W 128
#define SCREEN_H 64
#define SIZE 10
#define MOVE_SPEED 0.15f
#define ROT_SPEED 0.01f
#define GRAVITY 0.01f
#define JUMP_FORCE 0.3f
#define GROUND_Y 2.0f

typedef struct { float x,y,z; } Vec3;

Vec3 cube[8] = {
    {-1,-1,-1}, {1,-1,-1}, {1,1,-1}, {-1,1,-1},
    {-1,-1,1},  {1,-1,1},  {1,1,1},  {-1,1,1}
};

int edges[12][2] = {
    {0,1},{1,2},{2,3},{3,0},
    {4,5},{5,6},{6,7},{7,4},
    {0,4},{1,5},{2,6},{3,7}
};

// Murs de collision (zone de -10 à 10 en x et z)
#define WALL_MIN -10.0f
#define WALL_MAX 10.0f

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

// Vérification des collisions avec les murs
void check_collision_walls(Vec3 *cam) {
    if(cam->x < WALL_MIN) cam->x = WALL_MIN;
    if(cam->x > WALL_MAX) cam->x = WALL_MAX;
    if(cam->z < WALL_MIN) cam->z = WALL_MIN;
    if(cam->z > WALL_MAX) cam->z = WALL_MAX;
}

// Vérifie et résout les collisions avec le cube
void check_collision_cube(Vec3 *cam, float ax, float ay) {
    // Limites du cube (centré à l'origine, taille 2x2x2)
    float cube_min_x = -1.0f, cube_max_x = 1.0f;
    float cube_min_y = -1.0f, cube_max_y = 1.0f;
    float cube_min_z = -1.0f, cube_max_z = 1.0f;

    // Si la caméra est à l'intérieur du cube, on la repousse
    if (cam->x >= cube_min_x && cam->x <= cube_max_x &&
        cam->y >= cube_min_y && cam->y <= cube_max_y &&
        cam->z >= cube_min_z && cam->z <= cube_max_z) {

        // Calcul du vecteur de déplacement pour sortir du cube
        float push_x = 0.0f, push_y = 0.0f, push_z = 0.0f;

        // On calcule la distance minimale pour sortir du cube sur chaque axe
        if (cam->x < (cube_min_x + 0.1f)) push_x = cube_min_x - cam->x;
        if (cam->x > (cube_max_x - 0.1f)) push_x = cube_max_x - cam->x;
        if (cam->y < (cube_min_y + 0.1f)) push_y = cube_min_y - cam->y;
        if (cam->y > (cube_max_y - 0.1f)) push_y = cube_max_y - cam->y;
        if (cam->z < (cube_min_z + 0.1f)) push_z = cube_min_z - cam->z;
        if (cam->z > (cube_max_z - 0.1f)) push_z = cube_max_z - cam->z;

        // On applique le déplacement minimal pour sortir du cube
        if (fabs(push_x) > fabs(push_z)) {
            if (fabs(push_x) > fabs(push_y)) {
                cam->x += push_x;
            } else {
                cam->y += push_y;
            }
        } else {
            if (fabs(push_z) > fabs(push_y)) {
                cam->z += push_z;
            } else {
                cam->y += push_y;
            }
        }
    }
}

int main(void) {
    Vec3 cam = {0, -GROUND_Y, -5};
    float ax=0, ay=0;
    float velocity_y = 0.0f;
    int on_ground = 0;

    while(1) {
        dclear(C_WHITE);

        // Draw cube edges
        for(int i=0; i<12; i++) {
            int x1,y1,x2,y2;
            project(cube[edges[i][0]], cam, ax, ay, &x1,&y1);
            project(cube[edges[i][1]], cam, ax, ay, &x2,&y2);
            dline(x1,y1,x2,y2,C_BLACK);
        }

        // Afficher les infos de position
        char buf[32];
        sprintf(buf, "Y:%.1f", cam.y);
        dtext(2, 2, C_BLACK, buf);
        if(on_ground) dtext(2, 10, C_BLACK, "GROUND");

        dupdate();

        // Vérifier les touches sans bloquer
        clearevents();
        int key = KEYEV_NONE;

        // Boucle de mise à jour physique rapide
        for(int frame=0; frame<5; frame++) {
            // Vérifier s'il y a une touche pressée
            if(keydown(KEY_EXIT)) {
                return 1;
            }

            // Head rotation (inversé pour être naturel)
            if(keydown(KEY_LEFT))  ay += ROT_SPEED;
            if(keydown(KEY_RIGHT)) ay -= ROT_SPEED;
            if(keydown(KEY_UP))    ax += ROT_SPEED;
            if(keydown(KEY_DOWN))  ax -= ROT_SPEED;

            // Mouvement (8=avant, 2=arrière, 4=gauche, 6=droite)
            float sinA = sinf(ay), cosA = cosf(ay);

            // Mouvement avant/arrière
            if(keydown(KEY_8)) { cam.x -= sinA*MOVE_SPEED; cam.z += cosA*MOVE_SPEED; }
            if(keydown(KEY_2)) { cam.x += sinA*MOVE_SPEED; cam.z -= cosA*MOVE_SPEED; }

            // Mouvement latéral (normalisé)
            if(keydown(KEY_4)) { cam.x -= cosA*MOVE_SPEED; cam.z -= sinA*MOVE_SPEED; }
            if(keydown(KEY_6)) { cam.x += cosA*MOVE_SPEED; cam.z += sinA*MOVE_SPEED; }

            // Collision avec les murs
            check_collision_walls(&cam);

            // Saut (touche 7 ou 9)
            if((keydown(KEY_7) || keydown(KEY_9)) && on_ground) {
                velocity_y = -JUMP_FORCE;
                on_ground = 0;
            }

            // Gravité (s'applique toujours !)
            velocity_y += GRAVITY;
            cam.y += velocity_y;

            // Collision avec le sol
            if(cam.y >= GROUND_Y) {
                cam.y = GROUND_Y;
                velocity_y = 0.0f;
                on_ground = 1;
            } else {
                on_ground = 0;
            }

            // Vol libre (touche 3 pour descendre)
            if(keydown(KEY_3)) {
                cam.y += MOVE_SPEED;
                velocity_y = 0.0f;
            }

            // Collision avec le cube
            check_collision_cube(&cam, ax, ay);

            // Petite pause pour ne pas surcharger le CPU
            for(volatile int i=0; i<20000; i++);
        }
    }

    return 1;
}
