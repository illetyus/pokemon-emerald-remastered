#include "remaster/core.h"

#include <SDL3/SDL.h>

#include <stdio.h>
#include <string.h>

enum {
    WINDOW_WIDTH = 960,
    WINDOW_HEIGHT = 640,
    MAP_WIDTH = 8,
    MAP_HEIGHT = 8,
    TILE_SIZE = 64,
    MAP_ORIGIN_X = 64,
    MAP_ORIGIN_Y = 64
};

static const unsigned char kCollision[MAP_HEIGHT][MAP_WIDTH] = {
    {1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,1},
    {1,0,1,0,0,1,0,1},
    {1,0,1,0,0,1,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,1,1,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1}
};

static void apply_input(RemasterState *state, RemasterInput input)
{
    remaster_core_step(state, input);
}

static RemasterInput key_to_input(SDL_Keycode key)
{
    switch (key) {
    case SDLK_UP:
    case SDLK_W:
        return REMASTER_INPUT_MOVE_UP;
    case SDLK_DOWN:
    case SDLK_S:
        return REMASTER_INPUT_MOVE_DOWN;
    case SDLK_LEFT:
    case SDLK_A:
        return REMASTER_INPUT_MOVE_LEFT;
    case SDLK_RIGHT:
    case SDLK_D:
        return REMASTER_INPUT_MOVE_RIGHT;
    case SDLK_SPACE:
    case SDLK_RETURN:
        return REMASTER_INPUT_INTERACT;
    default:
        return REMASTER_INPUT_NONE;
    }
}

static RemasterInput gamepad_button_to_input(Uint8 button)
{
    switch (button) {
    case SDL_GAMEPAD_BUTTON_DPAD_UP:
        return REMASTER_INPUT_MOVE_UP;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        return REMASTER_INPUT_MOVE_DOWN;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
        return REMASTER_INPUT_MOVE_LEFT;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
        return REMASTER_INPUT_MOVE_RIGHT;
    case SDL_GAMEPAD_BUTTON_SOUTH:
        return REMASTER_INPUT_INTERACT;
    default:
        return REMASTER_INPUT_NONE;
    }
}

static RemasterInput touch_to_input(float x, float y)
{
    if (x > 0.70f)
        return REMASTER_INPUT_INTERACT;

    if (x < 0.35f) {
        if (y < 0.35f)
            return REMASTER_INPUT_MOVE_UP;
        if (y > 0.65f)
            return REMASTER_INPUT_MOVE_DOWN;
        return REMASTER_INPUT_MOVE_LEFT;
    }

    if (x < 0.70f)
        return REMASTER_INPUT_MOVE_RIGHT;

    return REMASTER_INPUT_NONE;
}

static void render_scene(SDL_Renderer *renderer, const RemasterState *state)
{
    int x;
    int y;

    SDL_SetRenderDrawColor(renderer, 22, 26, 32, 255);
    SDL_RenderClear(renderer);

    for (y = 0; y < MAP_HEIGHT; ++y) {
        for (x = 0; x < MAP_WIDTH; ++x) {
            SDL_FRect tile = {
                (float)(MAP_ORIGIN_X + x * TILE_SIZE),
                (float)(MAP_ORIGIN_Y + y * TILE_SIZE),
                (float)(TILE_SIZE - 2),
                (float)(TILE_SIZE - 2)
            };

            if (kCollision[y][x]) {
                SDL_SetRenderDrawColor(renderer, 58, 67, 76, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 93, 154, 91, 255);
            }

            SDL_RenderFillRect(renderer, &tile);
        }
    }

    {
        SDL_FRect event_tile = {
            (float)(MAP_ORIGIN_X + 3 * TILE_SIZE),
            (float)(MAP_ORIGIN_Y + 1 * TILE_SIZE),
            (float)(TILE_SIZE - 2),
            (float)(TILE_SIZE - 2)
        };

        if ((state->event_flags & 1u) != 0u)
            SDL_SetRenderDrawColor(renderer, 237, 185, 75, 255);
        else
            SDL_SetRenderDrawColor(renderer, 181, 116, 53, 255);

        SDL_RenderFillRect(renderer, &event_tile);
    }

    {
        const float inset = 12.0f;
        SDL_FRect player = {
            (float)(MAP_ORIGIN_X + state->tile_x * TILE_SIZE) + inset,
            (float)(MAP_ORIGIN_Y + state->tile_y * TILE_SIZE) + inset,
            (float)TILE_SIZE - (inset * 2.0f) - 2.0f,
            (float)TILE_SIZE - (inset * 2.0f) - 2.0f
        };

        if (state->encounter_pending)
            SDL_SetRenderDrawColor(renderer, 210, 74, 74, 255);
        else
            SDL_SetRenderDrawColor(renderer, 74, 139, 210, 255);

        SDL_RenderFillRect(renderer, &player);
    }

    SDL_RenderPresent(renderer);
}

static void update_title(SDL_Window *window, const RemasterState *state)
{
    char title[256];

    (void)snprintf(
        title,
        sizeof(title),
        "Pokemon Emerald Remastered — R0 SDL3 | tile=(%d,%d) steps=%u interactions=%u flags=0x%X encounter=%u",
        (int)state->tile_x,
        (int)state->tile_y,
        (unsigned int)state->step_count,
        (unsigned int)state->interaction_count,
        (unsigned int)state->event_flags,
        (unsigned int)state->encounter_pending
    );

    SDL_SetWindowTitle(window, title);
}

static int run_self_test(void)
{
    RemasterState state;
    RemasterState restored;
    uint8_t save_data[REMASTER_CORE_STATE_BYTES];
    uint64_t hash_before;

    remaster_core_init(&state);

    remaster_core_step(&state, REMASTER_INPUT_MOVE_UP);
    remaster_core_step(&state, REMASTER_INPUT_MOVE_RIGHT);
    remaster_core_step(&state, REMASTER_INPUT_MOVE_RIGHT);
    remaster_core_step(&state, REMASTER_INPUT_INTERACT);
    remaster_core_step(&state, REMASTER_INPUT_MOVE_DOWN);
    remaster_core_step(&state, REMASTER_INPUT_MOVE_DOWN);
    remaster_core_step(&state, REMASTER_INPUT_MOVE_DOWN);

    if (state.tile_x != 3 || state.tile_y != 4 ||
        state.step_count != 5 || state.interaction_count != 1 ||
        state.event_flags != 1u || state.encounter_pending != 1u) {
        fprintf(stderr, "SDL3 R0 self-test: scenario mismatch\n");
        return 1;
    }

    hash_before = remaster_core_state_hash(&state);

    if (!remaster_core_save(&state, save_data, sizeof(save_data))) {
        fprintf(stderr, "SDL3 R0 self-test: save failed\n");
        return 1;
    }

    remaster_core_init(&restored);

    if (!remaster_core_load(&restored, save_data, sizeof(save_data))) {
        fprintf(stderr, "SDL3 R0 self-test: load failed\n");
        return 1;
    }

    if (remaster_core_state_hash(&restored) != hash_before) {
        fprintf(stderr, "SDL3 R0 self-test: save/load hash mismatch\n");
        return 1;
    }

    remaster_core_step(&state, REMASTER_INPUT_MOVE_RIGHT);
    remaster_core_step(&restored, REMASTER_INPUT_MOVE_RIGHT);

    if (remaster_core_state_hash(&state) != remaster_core_state_hash(&restored)) {
        fprintf(stderr, "SDL3 R0 self-test: continuation diverged\n");
        return 1;
    }

    printf("R0 SDL3 self-test passed. state_hash=%llu\n",
           (unsigned long long)remaster_core_state_hash(&state));
    return 0;
}

int main(int argc, char **argv)
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    RemasterState state;
    uint8_t save_data[REMASTER_CORE_STATE_BYTES];
    bool has_save = false;
    bool running = true;

    if (argc > 1 && strcmp(argv[1], "--self-test") == 0)
        return run_self_test();

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow(
        "Pokemon Emerald Remastered — R0 SDL3",
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_RESIZABLE
    );

    if (window == NULL) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, NULL);
    if (renderer == NULL) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    (void)SDL_SetRenderVSync(renderer, 1);
    remaster_core_init(&state);

    SDL_Log("R0 controls: arrows/WASD move, Space/Enter interact, F5 save, F9 load, R reset, Esc quit.");

    while (running) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
                RemasterInput input;

                if (event.key.key == SDLK_ESCAPE) {
                    running = false;
                    continue;
                }

                if (event.key.key == SDLK_R) {
                    remaster_core_init(&state);
                    continue;
                }

                if (event.key.key == SDLK_F5) {
                    has_save = remaster_core_save(&state, save_data, sizeof(save_data)) != 0;
                    continue;
                }

                if (event.key.key == SDLK_F9) {
                    if (has_save)
                        (void)remaster_core_load(&state, save_data, sizeof(save_data));
                    continue;
                }

                input = key_to_input(event.key.key);
                if (input != REMASTER_INPUT_NONE)
                    apply_input(&state, input);
            } else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
                RemasterInput input = gamepad_button_to_input(event.gbutton.button);
                if (input != REMASTER_INPUT_NONE)
                    apply_input(&state, input);
            } else if (event.type == SDL_EVENT_FINGER_DOWN) {
                RemasterInput input = touch_to_input(event.tfinger.x, event.tfinger.y);
                if (input != REMASTER_INPUT_NONE)
                    apply_input(&state, input);
            }
        }

        update_title(window, &state);
        render_scene(renderer, &state);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
