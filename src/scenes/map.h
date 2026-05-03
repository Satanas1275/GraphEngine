#ifndef MAP_H
#define MAP_H

#include <gint/display.h>

#ifndef VEC3_DEFINED
typedef struct { float x, y, z; } Vec3;
#define VEC3_DEFINED
#endif

// Types d'objets
typedef enum { OBJ_CUBE } ObjType;

// Définition d'un objet de scène
typedef struct {
    ObjType type;
    Vec3 pos;
    float scale;
} SceneObject;

// Limites du monde
#define WALL_MIN -10.0f
#define WALL_MAX 10.0f

// Hauteur du sol
#define GROUND_Y 2.0f

// Position initiale du joueur
static const Vec3 PLAYER_START = {0, -GROUND_Y, -5};

// Objets de la scène (ajoute des entrées ici pour ajouter des cubes)
static const SceneObject SCENE_OBJECTS[] = {
    {.type = OBJ_CUBE, .pos = {0.0f, 0.0f, 0.0f}, .scale = 1.0f},
    {.type = OBJ_CUBE, .pos = {3.0f, GROUND_Y - 1.0f, 0.0f}, .scale = 1.0f},
};
#define SCENE_OBJECT_COUNT ((int)(sizeof(SCENE_OBJECTS) / sizeof(SceneObject)))

// Paramètres du jeu
#define GRAVITY 0.01f
#define JUMP_FORCE 0.3f
#define MOVE_SPEED 0.15f

#endif
