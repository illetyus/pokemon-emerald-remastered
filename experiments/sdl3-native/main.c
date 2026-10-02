#include "remaster/core.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

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

enum {
    R0_SAVE_MAGIC_SIZE = 4,
    R0_SAVE_HASH_SIZE = 8,
    R0_SAVE_FILE_SIZE = R0_SAVE_MAGIC_SIZE + REMASTER_CORE_STATE_BYTES + R0_SAVE_HASH_SIZE
};

typedef struct R0LifecycleContext {
    RemasterState *state;
    char save_path[1024];
} R0LifecycleContext;

typedef struct R0PerfWindow {
    Uint64 window_start_ns;
    Uint64 previous_frame_ns;
    double total_frame_ms;
    double worst_frame_ms;
    uint32_t frame_count;
} R0PerfWindow;

static const uint8_t kSaveMagic[R0_SAVE_MAGIC_SIZE] = { 'R', '0', 'S', '1' };

static void write_u64_le(uint8_t *dst, uint64_t value)
{
    unsigned int i;

    for (i = 0; i < 8; ++i)
        dst[i] = (uint8_t)((value >> (i * 8u)) & UINT64_C(0xff));
}

static uint64_t read_u64_le(const uint8_t *src)
{
    uint64_t value = 0;
    unsigned int i;

    for (i = 0; i < 8; ++i)
        value |= ((uint64_t)src[i]) << (i * 8u);

    return value;
}

static bool save_state_to_disk(const RemasterState *state, const char *path)
{
    FILE *fp;
    uint8_t file_data[R0_SAVE_FILE_SIZE];
    uint64_t hash;

    if (state == NULL || path == NULL || path[0] == '\0')
        return false;

    memcpy(file_data, kSaveMagic, R0_SAVE_MAGIC_SIZE);

    if (!remaster_core_save(
            state,
            file_data + R0_SAVE_MAGIC_SIZE,
            REMASTER_CORE_STATE_BYTES))
        return false;

    hash = remaster_core_state_hash(state);
    write_u64_le(
        file_data + R0_SAVE_MAGIC_SIZE + REMASTER_CORE_STATE_BYTES,
        hash
    );

    fp = fopen(path, "wb");
    if (fp == NULL)
        return false;

    if (fwrite(file_data, 1, sizeof(file_data), fp) != sizeof(file_data)) {
        fclose(fp);
        return false;
    }

    if (fflush(fp) != 0) {
        fclose(fp);
        return false;
    }

    if (fclose(fp) != 0)
        return false;

    SDL_Log(
        "R0 persistent save complete: hash=%llu path=%s",
        (unsigned long long)hash,
        path
    );
    return true;
}

static bool load_state_from_disk(RemasterState *state, const char *path)
{
    FILE *fp;
    uint8_t file_data[R0_SAVE_FILE_SIZE];
    uint64_t expected_hash;
    uint64_t actual_hash;

    if (state == NULL || path == NULL || path[0] == '\0')
        return false;

    fp = fopen(path, "rb");
    if (fp == NULL)
        return false;

    if (fread(file_data, 1, sizeof(file_data), fp) != sizeof(file_data)) {
        fclose(fp);
        return false;
    }

    if (fgetc(fp) != EOF) {
        fclose(fp);
        return false;
    }

    fclose(fp);

    if (memcmp(file_data, kSaveMagic, R0_SAVE_MAGIC_SIZE) != 0)
        return false;

    if (!remaster_core_load(
            state,
            file_data + R0_SAVE_MAGIC_SIZE,
            REMASTER_CORE_STATE_BYTES))
        return false;

    expected_hash = read_u64_le(
        file_data + R0_SAVE_MAGIC_SIZE + REMASTER_CORE_STATE_BYTES
    );
    actual_hash = remaster_core_state_hash(state);

    if (expected_hash != actual_hash) {
        SDL_Log(
            "R0 persistent save rejected: expected_hash=%llu actual_hash=%llu",
            (unsigned long long)expected_hash,
            (unsigned long long)actual_hash
        );
        remaster_core_init(state);
        return false;
    }

    SDL_Log(
        "R0 persistent load complete: hash=%llu path=%s",
        (unsigned long long)actual_hash,
        path
    );
    return true;
}

static bool SDLCALL lifecycle_event_watch(void *userdata, SDL_Event *event)
{
    R0LifecycleContext *context = (R0LifecycleContext *)userdata;

    if (context == NULL || context->state == NULL || event == NULL)
        return true;

    switch (event->type) {
    case SDL_EVENT_WILL_ENTER_BACKGROUND:
        SDL_Log("R0 lifecycle: WILL_ENTER_BACKGROUND");
        (void)save_state_to_disk(context->state, context->save_path);
        break;
    case SDL_EVENT_DID_ENTER_BACKGROUND:
        SDL_Log("R0 lifecycle: DID_ENTER_BACKGROUND");
        break;
    case SDL_EVENT_WILL_ENTER_FOREGROUND:
        SDL_Log("R0 lifecycle: WILL_ENTER_FOREGROUND");
        break;
    case SDL_EVENT_DID_ENTER_FOREGROUND:
        SDL_Log(
            "R0 lifecycle: DID_ENTER_FOREGROUND hash=%llu",
            (unsigned long long)remaster_core_state_hash(context->state)
        );
        break;
    case SDL_EVENT_TERMINATING:
        SDL_Log("R0 lifecycle: TERMINATING");
        (void)save_state_to_disk(context->state, context->save_path);
        break;
    default:
        break;
    }

    return true;
}

static bool initialize_persistent_path(R0LifecycleContext *context)
{
    char *pref_path;
    int written;

    if (context == NULL)
        return false;

    pref_path = SDL_GetPrefPath("illetyus", "pokemon-emerald-remastered-r0");
    if (pref_path == NULL)
        return false;

    written = snprintf(
        context->save_path,
        sizeof(context->save_path),
        "%sr0_state.bin",
        pref_path
    );
    SDL_free(pref_path);

    return written > 0 && (size_t)written < sizeof(context->save_path);
}


static void perf_window_init(R0PerfWindow *perf)
{
    Uint64 now;

    if (perf == NULL)
        return;

    now = SDL_GetTicksNS();
    perf->window_start_ns = now;
    perf->previous_frame_ns = now;
    perf->total_frame_ms = 0.0;
    perf->worst_frame_ms = 0.0;
    perf->frame_count = 0;
}

static void perf_window_tick(R0PerfWindow *perf)
{
    const Uint64 report_interval_ns = UINT64_C(5000000000);
    Uint64 now;
    Uint64 frame_ns;
    Uint64 elapsed_ns;
    double frame_ms;
    double elapsed_seconds;
    double fps;
    double average_ms;

    if (perf == NULL)
        return;

    now = SDL_GetTicksNS();
    frame_ns = now - perf->previous_frame_ns;
    perf->previous_frame_ns = now;

    frame_ms = (double)frame_ns / 1000000.0;
    perf->total_frame_ms += frame_ms;
    if (frame_ms > perf->worst_frame_ms)
        perf->worst_frame_ms = frame_ms;
    perf->frame_count++;

    elapsed_ns = now - perf->window_start_ns;
    if (elapsed_ns < report_interval_ns || perf->frame_count == 0)
        return;

    elapsed_seconds = (double)elapsed_ns / 1000000000.0;
    fps = (double)perf->frame_count / elapsed_seconds;
    average_ms = perf->total_frame_ms / (double)perf->frame_count;

    SDL_Log(
        "R0 PERF renderer=SDL3 fps=%.2f avg_frame_ms=%.3f worst_frame_ms=%.3f frames=%u",
        fps,
        average_ms,
        perf->worst_frame_ms,
        (unsigned int)perf->frame_count
    );

    perf->window_start_ns = now;
    perf->total_frame_ms = 0.0;
    perf->worst_frame_ms = 0.0;
    perf->frame_count = 0;
}

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
    uint32_t events;

    remaster_core_init(&state);

    events = remaster_core_step(&state, REMASTER_INPUT_MOVE_UP);
    if (events != REMASTER_EVENT_BLOCKED) {
        fprintf(stderr, "SDL3 R0 self-test: blocked-event mismatch\n");
        return 1;
    }
    remaster_core_step(&state, REMASTER_INPUT_MOVE_RIGHT);
    remaster_core_step(&state, REMASTER_INPUT_MOVE_RIGHT);
    events = remaster_core_step(&state, REMASTER_INPUT_INTERACT);
    if (events != (REMASTER_EVENT_INTERACTED | REMASTER_EVENT_FLAG_SET)) {
        fprintf(stderr, "SDL3 R0 self-test: interaction-event mismatch\n");
        return 1;
    }
    remaster_core_step(&state, REMASTER_INPUT_MOVE_DOWN);
    remaster_core_step(&state, REMASTER_INPUT_MOVE_DOWN);
    events = remaster_core_step(&state, REMASTER_INPUT_MOVE_DOWN);
    if (events != (REMASTER_EVENT_MOVED | REMASTER_EVENT_ENCOUNTER)) {
        fprintf(stderr, "SDL3 R0 self-test: encounter-event mismatch\n");
        return 1;
    }

    if (state.tile_x != 3 || state.tile_y != 4 ||
        state.step_count != 5 || state.interaction_count != 1 ||
        state.event_flags != 1u || state.encounter_pending != 1u) {
        fprintf(stderr, "SDL3 R0 self-test: scenario mismatch\n");
        return 1;
    }

    hash_before = remaster_core_state_hash(&state);

    if (hash_before != UINT64_C(7218695048241891488)) {
        fprintf(stderr, "SDL3 R0 self-test: canonical state hash mismatch\n");
        return 1;
    }

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
    R0LifecycleContext lifecycle_context;
    R0PerfWindow perf_window;
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
    perf_window_init(&perf_window);

    lifecycle_context.state = &state;
    lifecycle_context.save_path[0] = '\0';

    if (initialize_persistent_path(&lifecycle_context)) {
        (void)load_state_from_disk(&state, lifecycle_context.save_path);
        if (!SDL_AddEventWatch(lifecycle_event_watch, &lifecycle_context))
            SDL_Log("R0 lifecycle watcher could not be registered: %s", SDL_GetError());
    } else {
        SDL_Log("R0 persistent path unavailable: %s", SDL_GetError());
    }

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
                    if (lifecycle_context.save_path[0] != '\0')
                        (void)save_state_to_disk(&state, lifecycle_context.save_path);
                    continue;
                }

                if (event.key.key == SDLK_F9) {
                    if (has_save) {
                        (void)remaster_core_load(&state, save_data, sizeof(save_data));
                    } else if (lifecycle_context.save_path[0] != '\0') {
                        (void)load_state_from_disk(&state, lifecycle_context.save_path);
                    }
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
        perf_window_tick(&perf_window);
    }

    if (lifecycle_context.save_path[0] != '\0') {
        (void)save_state_to_disk(&state, lifecycle_context.save_path);
        SDL_RemoveEventWatch(lifecycle_event_watch, &lifecycle_context);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
