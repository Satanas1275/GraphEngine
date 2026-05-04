#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/timer.h>
#include <gint/clock.h>
#include <stdio.h>

#include "engine/player.h"
#include "engine/renderer.h"

// ─── Config ──────────────────────────────────────────────────────────────────

#define TARGET_FPS  30
#define SUBSTEPS     4   // sous-pas physique par frame

// ─── Main ────────────────────────────────────────────────────────────────────

int main(void) {
    Player p = player_init();

    while (1) {
        // ── Rendu ──────────────────────────────────────────────────────────
        dclear(C_WHITE);

        Vec3 eye = player_eye(&p);
        render_scene(eye, p.pitch, p.yaw);

        // HUD debug
        char buf[40];
        sprintf(buf, "%.1f %.1f %.1f", p.feet.x, p.feet.y, p.feet.z);
        dtext(1, 1,  C_BLACK, buf);
        dtext(1, 9,  C_BLACK, p.on_ground ? "GND" : "AIR");

        dupdate();

        // ── Inputs + Physique ──────────────────────────────────────────────
        clearevents();

        for (int s = 0; s < SUBSTEPS; s++) {
            if (keydown(KEY_EXIT)) return 1;
            player_update(&p);
        }

        // ── Frame pacing ───────────────────────────────────────────────────
        sleep_ms(1000 / TARGET_FPS);
    }

    return 1;
}
