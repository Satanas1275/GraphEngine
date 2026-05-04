#ifndef RENDERER_H
#define RENDERER_H

#include <gint/display.h>
#include "math3d.h"

// ─── Géométrie du cube ───────────────────────────────────────────────────────

// 8 sommets d'un cube unitaire (demi-taille 1)
static const Vec3 CUBE_VERTS[8] = {
    {-1,-1,-1}, { 1,-1,-1}, { 1, 1,-1}, {-1, 1,-1},
    {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1}
};

// 6 faces (4 sommets chacune, ordre anti-horaire vu de l'extérieur)
static const int CUBE_FACES[6][4] = {
    {0,1,2,3}, // arrière  (-Z)
    {5,4,7,6}, // avant    (+Z)
    {4,0,3,7}, // gauche   (-X)
    {1,5,6,2}, // droite   (+X)
    {4,5,1,0}, // bas      (-Y)
    {3,2,6,7}, // haut     (+Y)
};

// Normales des faces (pré-calculées)
static const Vec3 CUBE_NORMALS[6] = {
    { 0, 0,-1},
    { 0, 0, 1},
    {-1, 0, 0},
    { 1, 0, 0},
    { 0,-1, 0},
    { 0, 1, 0},
};

// ─── Rendu d'un cube ─────────────────────────────────────────────────────────

// Dessine un cube avec face culling (on n'affiche que les faces visibles)
static inline void render_cube(Vec3 pos, float scale,
                                Vec3 cam_pos, float ax, float ay)
{
    // Direction depuis le centre du cube vers la caméra
    // (pour le face culling on compare dot(normal, cam_dir) > 0)
    Vec3 cam_to_cube = vec3_sub(cam_pos, pos);

    for (int f = 0; f < 6; f++) {
        // Face culling : on skip les faces qui ne font pas face à la caméra
        if (vec3_dot(CUBE_NORMALS[f], cam_to_cube) <= 0.0f) continue;

        // Projeter les 4 sommets de la face
        int sx[4], sy[4];
        int visible = 1;
        for (int v = 0; v < 4; v++) {
            Vec3 world = {
                CUBE_VERTS[CUBE_FACES[f][v]].x * scale + pos.x,
                CUBE_VERTS[CUBE_FACES[f][v]].y * scale + pos.y,
                CUBE_VERTS[CUBE_FACES[f][v]].z * scale + pos.z,
            };
            if (!project_point(world, cam_pos, ax, ay, &sx[v], &sy[v])) {
                visible = 0;
                break;
            }
        }
        if (!visible) continue;

        // Dessiner les 4 arêtes de la face
        for (int v = 0; v < 4; v++) {
            int nv = (v + 1) % 4;
            dline(sx[v], sy[v], sx[nv], sy[nv], C_BLACK);
        }
    }
}

// ─── Rendu de la scène ───────────────────────────────────────────────────────

#include "../scenes/map.h"

static inline void render_scene(Vec3 cam_pos, float ax, float ay) {
    // Tri des objets par distance (painter's algorithm simplifié)
    // Sur SH3 on évite le tri dynamique — on parcourt juste dans l'ordre
    // (suffisant pour une scène avec peu d'objets)
    for (int i = 0; i < SCENE_OBJECT_COUNT; i++) {
        if (SCENE_OBJECTS[i].type == OBJ_CUBE) {
            render_cube(SCENE_OBJECTS[i].pos, SCENE_OBJECTS[i].scale,
                        cam_pos, ax, ay);
        }
    }
}

#endif // RENDERER_H
