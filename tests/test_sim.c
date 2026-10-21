// Runs the simulation in a closed box and checks it stays sane: no NaN or
// negative levels, walls untouched, mass kept, mercury sinks to the bottom.
#define NO_MAIN
#include "../main.c"

#define STEPS 5000

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); failures++; } } while (0)

static double total_mass(Cell env[]){
    double m = 0;
    for (int i = 0; i < COLS * ROWS; i++)
        if (env[i].type != -1) m += env[i].fill_level;
    return m;
}

int main(void){
    static Cell env[COLS * ROWS];
    prep_env(env);

    int walls = 0;
    for (int y = 0; y < ROWS; y++)
        for (int x = 0; x < COLS; x++){
            int idx = y * COLS + x;
            if (x == 0 || y == 0 || x == COLS - 1 || y == ROWS - 1){
                env[idx] = (Cell){-1, 0.0, x, y, 0, wall};
                walls++;
            } else if (y >= 2 && y < 12){
                // mercury on top, oil under it, then a stripe of every other liquid
                int t = y < 4 ? 19 : (y < 8 ? 5 : x % 19);
                env[idx] = (Cell){t, 1.0, x, y, liquids[t].weight, liquids[t].color};
            }
        }

    double start = total_mass(env), prev = start;
    for (int it = 0; it < STEPS && !failures; it++){
        // same order as the main loop
        simulation_step(env);
        mix_step(env);
        density_step(env);

        int wall_count = 0;
        for (int i = 0; i < COLS * ROWS; i++){
            if (env[i].type == -1){ wall_count++; continue; }
            CHECK(!isnan(env[i].fill_level) && env[i].fill_level >= 0,
                  "step %d: cell %d has level %g", it, i, env[i].fill_level);
        }
        CHECK(wall_count == walls, "step %d: %d walls, expected %d", it, wall_count, walls);

        double m = total_mass(env);
        CHECK(m <= prev + 1e-9, "step %d: mass grew from %.9f to %.9f", it, prev, m);
        prev = m;
    }

    // Levels under MIN_FLOW are cleared (as in the article), so a little mass evaporates.
    double end = total_mass(env);
    CHECK(end >= start * 0.98, "mass fell from %.3f to %.3f", start, end);

    for (int x = 1; x < COLS - 1; x++)
        CHECK(env[(ROWS - 2) * COLS + x].type == 19, "bottom cell %d is type %d, expected mercury",
              x, env[(ROWS - 2) * COLS + x].type);

    printf("%s: %d steps, mass %.3f -> %.3f (%.2f%% evaporated)\n", failures ? "FAILED" : "ok",
           STEPS, start, end, 100 * (start - end) / start);
    return failures != 0;
}
