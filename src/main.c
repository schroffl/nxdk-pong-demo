/*
 * This sample provides a very basic demonstration of 3D rendering on the Xbox,
 * using pbkit. Based on the pbkit demo sources.
 */
#include <hal/video.h>
#include <hal/xbox.h>
#include <math.h>
#include <pbkit/pbkit.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <xboxkrnl/xboxkrnl.h>
#include <hal/debug.h>
#include <windows.h>
#include <SDL.h>

#include "util.h"
#include "./matrix.h"
#include "./vec2.h"
#include "./game.h"

static uint32_t *alloc_vertices;
static uint32_t *alloc_vertices_box;
static uint32_t  num_vertices;

static float     m_viewport[4][4];
static float     m_scale[4][4];
static float     m_box_scale[4][4];
static float     m_translate[4][4];

// Matrix for storing intermediate results
static float     m_wrkmem[4][4];

// The final transformation matrix
static float     m_transform[4][4];

typedef struct {
    float pos[3];
} __attribute__((packed)) Vertex;

struct {
    bool up;
    bool down;
    bool left;
    bool right;
} controls;

float friction = 0.00f;
float max_velocity = 0.02f;
float player_speed = 0.02f;
float ball_radius = 0.04f;

Vec2 box_size = { 0.01f, 0.2f };
static float box_offset_x;

static const Rect bounds = {
    { -1.0f,  1.0f },
    {  1.0f, -1.0f }
};

Vec2 ball_velocity = {0.0f, 0.0f};

#define CIRCLE_VERTICES 64
static Vertex verts[CIRCLE_VERTICES];

static Vertex box[] = {
    { -1.0,  1.0, 0.0 },
    {  1.0,  1.0, 0.0 },
    {  1.0, -1.0, 0.0 },
    { -1.0, -1.0, 0.0 },
};

#define MASK(mask, val) (((val) << (ffs(mask)-1)) & (mask))

static void init_shader(void);
static void set_attrib_pointer(unsigned int index, unsigned int format, unsigned int size, unsigned int stride, const void* data);
static void draw_arrays(unsigned int mode, int start, int count);
static void draw_with_matrix(unsigned int mode, int start, int count, float matrix[4][4], uint32_t *verts);
static void draw_player(float x, float y);
static void draw_ball(Vec2 pos);

static void generate_circle_vertices(Vertex *out, size_t n_vertices);
static void ball_physics();

/* Main program function */
int main(void)
{
    uint32_t *p;
    int       i, status;
    int       width, height;
    int       start, last, now;
    int       fps, frames, frames_total;

    SDL_GameController *pad = NULL;

    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);

    bool sdl_init = SDL_Init(SDL_INIT_GAMECONTROLLER) == 0;

    if (!sdl_init) {
        pb_draw_text_screen();
        debugPrint("SDL_Init failed: %s\n", SDL_GetError());
        Sleep(2000);
        return 1;
    }

    pad = SDL_GameControllerOpen(0);
    if (pad == NULL) {
        debugPrint("Failed to open gamecontroller 0\n");
        Sleep(2000);
        return 1;
    }

    if (SDL_NumJoysticks() < 1) {
        debugPrint("Please connect gamepad\n");
        Sleep(2000);
        return 1;
    }

    if ((status = pb_init())) {
        debugPrint("pb_init Error %d\n", status);
        Sleep(2000);
        return 1;
    }

    pb_show_front_screen();

    /* Basic setup */
    width = pb_back_buffer_width();
    height = pb_back_buffer_height();

    /* Load constant rendering things (shaders, geometry) */
    init_shader();
    num_vertices = sizeof(verts)/sizeof(verts[0]);
    generate_circle_vertices(verts, num_vertices);

    alloc_vertices = MmAllocateContiguousMemoryEx(sizeof(verts), 0, 0x3ffb000, 0, 0x404);
    memcpy(alloc_vertices, verts, sizeof(verts));

    alloc_vertices_box = MmAllocateContiguousMemoryEx(sizeof(box), 0, 0x3ffb000, 0, 0x404);
    memcpy(alloc_vertices_box, box, sizeof(box));

    matrix_viewport(m_viewport, 0, 0, width, height, 0, 65536.0f);
    matrix_scale(m_scale, ball_radius, ball_radius, 1.0f);
    matrix_scale(m_box_scale, box_size.x, box_size.y, 1.0f);
    box_offset_x = 1.0f - box_size.x / 2.0f;

    /* Setup to determine frames rendered every second */
    start = now = last = GetTickCount();
    frames_total = frames = fps = 0;

    while(1) {
        pb_wait_for_vbl();
        pb_reset();
        pb_target_back_buffer();

        /* Clear depth & stencil buffers */
        pb_erase_depth_stencil_buffer(0, 0, width, height);
        pb_fill(0, 0, width, height, 0x00000000);
        pb_erase_text_screen();

        SDL_GameControllerUpdate();

        controls.up = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_UP) > 0;
        controls.down = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN) > 0;
        controls.left = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_LEFT) > 0;
        controls.right = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) > 0;

        ball_physics();

        while(pb_busy()) {
            /* Wait for completion... */
        }

        switch (game_state.state) {
            case PlayerLeftWon:
                pb_print("Player Left Won!\nPress any key to restart");
                pb_draw_text_screen();
                break;

            case PlayerRightWon:
                pb_print("Player Right Won!\nPress any key to restart");
                pb_draw_text_screen();
                break;

            default:
                pb_print("Left %d - %d Right\n", game_state.player_left_points, game_state.player_right_points);
                pb_draw_text_screen();
                draw_ball(game_state.ball_pos);
                draw_player(-box_offset_x, game_state.player_left_pos);
                draw_player(box_offset_x, game_state.player_right_pos);
        }


        while(pb_busy()) {
            /* Wait for completion... */
        }

        /* Swap buffers (if we can) */
        while (pb_finished()) {
            /* Not ready to swap yet */
        }

        frames++;
        frames_total++;

        /* Latch FPS counter every second */
        now = GetTickCount();
        if ((now-last) > 1000) {
            fps = frames;
            frames = 0;
            last = now;
        }
    }

    /* Unreachable cleanup code */
    MmFreeContiguousMemory(alloc_vertices);
    pb_show_debug_screen();
    pb_kill();
    return 0;
}


/* Load the shader we will render with */
static void init_shader(void)
{
    uint32_t *p;
    int       i;

    /* Setup vertex shader */
    uint32_t vs_program[] = {
#include "vs.inl"
    };

    p = pb_begin();

    /* Set run address of shader */
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_START, 0);

    /* Set execution mode */
    p = pb_push1(p, NV097_SET_TRANSFORM_EXECUTION_MODE,
            MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_MODE_PROGRAM)
            | MASK(NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE, NV097_SET_TRANSFORM_EXECUTION_MODE_RANGE_MODE_PRIV));

    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN, 0);

    pb_end(p);

    /* Set cursor for program upload */
    p = pb_begin();
    p = pb_push1(p, NV097_SET_TRANSFORM_PROGRAM_LOAD, 0);
    pb_end(p);

    /* Copy program instructions (16-bytes each) */
    for (i=0; i<sizeof(vs_program)/16; i++) {
        p = pb_begin();
        pb_push(p++, NV097_SET_TRANSFORM_PROGRAM, 4);
        memcpy(p, &vs_program[i*4], 4*4);
        p+=4;
        pb_end(p);
    }

    /* Setup fragment shader */
    p = pb_begin();
#include "ps.inl"
    pb_end(p);
}

/* Set an attribute pointer */
static void set_attrib_pointer(unsigned int index, unsigned int format, unsigned int size, unsigned int stride, const void* data)
{
    uint32_t *p = pb_begin();
    p = pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_FORMAT + index*4,
            MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE, format) | \
            MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_SIZE, size) |  \
            MASK(NV097_SET_VERTEX_DATA_ARRAY_FORMAT_STRIDE, stride));
    p = pb_push1(p, NV097_SET_VERTEX_DATA_ARRAY_OFFSET + index*4, (uint32_t)data & 0x03ffffff);
    pb_end(p);
}

/* Send draw commands for the triangles */
static void draw_arrays(unsigned int mode, int start, int count)
{
    uint32_t *p = pb_begin();
    p = pb_push1(p, NV097_SET_BEGIN_END, mode);

    p = pb_push1(p, 0x40000000|NV097_DRAW_ARRAYS, //bit 30 means all params go to same register 0x1810
            MASK(NV097_DRAW_ARRAYS_COUNT, (count-1)) | MASK(NV097_DRAW_ARRAYS_START_INDEX, start));

    p = pb_push1(p, NV097_SET_BEGIN_END, NV097_SET_BEGIN_END_OP_END);
    pb_end(p);
}

static void draw_with_matrix(unsigned int mode, int start, int count, float matrix[4][4], uint32_t *verts) {
    uint32_t *p;

    /* Send shader constants */
    p = pb_begin();

    /* Set shader constants cursor at C0 */
    p = pb_push1(p, NV097_SET_TRANSFORM_CONSTANT_LOAD, 96);

    /* Send the transformation matrix */
    pb_push(p++, NV097_SET_TRANSFORM_CONSTANT, 16);
    memcpy(p, matrix, 16*4); p+=16;

    pb_end(p);
    p = pb_begin();

    /* Clear all attributes */
    pb_push(p++, NV097_SET_VERTEX_DATA_ARRAY_FORMAT,16);
    for(int i = 0; i < 16; i++) {
        *(p++) = NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F;
    }
    pb_end(p);

    /* Set vertex position attribute */
    set_attrib_pointer(0, NV097_SET_VERTEX_DATA_ARRAY_FORMAT_TYPE_F,
            3, sizeof(Vertex), verts);
    draw_arrays(mode, start, count);
}

static void draw_ball(Vec2 pos) {
    matrix_translate(m_translate, pos.x, pos.y, 0.0f);
    matrix_multiply(m_wrkmem, m_scale, m_translate);
    matrix_multiply(m_transform, m_wrkmem, m_viewport);
    draw_with_matrix(NV097_SET_BEGIN_END_OP_TRIANGLE_FAN, 0, num_vertices, m_transform, &alloc_vertices[0]);
}

static void draw_player(float x, float y) {
    matrix_translate(m_translate, x, y, 0.0f);
    matrix_multiply(m_wrkmem, m_box_scale, m_translate);
    matrix_multiply(m_transform, m_wrkmem, m_viewport);
    draw_with_matrix(NV097_SET_BEGIN_END_OP_TRIANGLE_FAN, 0, 4, m_transform, &alloc_vertices_box[0]);
}

static void generate_circle_vertices(Vertex *out, size_t n_vertices) {
    int i = 0;

    while (i < n_vertices) {
        float progress = ((float) i) / ((float) n_vertices);
        float phi = progress * M_PI * 2.0f;

        // It's important to have proper winding here, which is why we
        // multiply the y-component by -1. The back-face seems to get
        // culled otherwise.
        out[i].pos[0] = cos(phi);
        out[i].pos[1] = -sin(phi);
        out[i].pos[2] = 1.0f;

        i += 1;
    }
}

static void ball_physics() {
    switch (game_state.state) {
        case PlayerLeftBall:
            Vec2_set(&ball_velocity, -max_velocity, 0.0f);
            game_state.state = Playing;
            break;

        case PlayerRightBall:
            Vec2_set(&ball_velocity, max_velocity, 0.0f);
            game_state.state = Playing;
            break;

        default: {}
    }

    switch (game_state.state) {
        case PlayerLeftWon:
        case PlayerRightWon:
            if (controls.up || controls.left || controls.right || controls.down) {
                restart_game();
            }

            break;

        default:
            if (controls.up) {
                game_state.player_left_pos += player_speed;
            } else if (controls.down) {
                game_state.player_left_pos -= player_speed;
            }

            if (controls.left) {
                game_state.player_right_pos += player_speed;
            } else if (controls.right) {
                game_state.player_right_pos -= player_speed;
            }

            game_state.player_left_pos = clampf(game_state.player_left_pos, -1.0f + box_size.y, 1.0f - box_size.y);
            game_state.player_right_pos = clampf(game_state.player_right_pos, -1.0f + box_size.y, 1.0f - box_size.y);
    }

    ball_velocity.x = clampf(ball_velocity.x, -max_velocity, max_velocity);
    ball_velocity.y = clampf(ball_velocity.y, -max_velocity, max_velocity);

    Vec2_multiply_scalar(&ball_velocity, ball_velocity, 1.0f - friction);
    Vec2_add(&game_state.ball_pos, game_state.ball_pos, ball_velocity);

    Vec2 ball_pos = game_state.ball_pos;

    /**
     * Collision detection and resolution
     */

    Vec2 top_left = bounds.top_left;
    Vec2 bottom_right = bounds.bottom_right;

    if (ball_pos.y - ball_radius < bottom_right.y) {
        // Bottom border
        ball_velocity.y *= -1.0f;
        game_state.ball_pos.y = bottom_right.y + ball_radius;
    } else if (ball_pos.y + ball_radius > top_left.y) {
        // Top border
        ball_velocity.y *= -1.0f;
        game_state.ball_pos.y = top_left.y - ball_radius;
    }

    if (ball_pos.x < -box_offset_x + ball_radius + box_size.x) {
        if (ball_pos.y < game_state.player_left_pos + box_size.y && ball_pos.y > game_state.player_left_pos - box_size.y) {
            ball_velocity.x *= -1.0f;
            game_state.ball_pos.x = -box_offset_x + ball_radius + box_size.x;

            float dist = (game_state.ball_pos.y - game_state.player_left_pos) / box_size.y;
            ball_velocity.y += dist * max_velocity * 0.5f;
        }
    } else if (ball_pos.x > box_offset_x - ball_radius - box_size.x) {
        if (ball_pos.y < game_state.player_right_pos + box_size.y && ball_pos.y > game_state.player_right_pos - box_size.y) {
            ball_velocity.x *= -1.0f;
            game_state.ball_pos.x = box_offset_x - ball_radius - box_size.x;

            float dist = (game_state.ball_pos.y - game_state.player_right_pos) / box_size.y;
            ball_velocity.y += dist * max_velocity * 0.5f;
        }
    }

    if (ball_pos.x - ball_radius < top_left.x) {
        handle_goal(LeftSide);
        Vec2_set(&ball_velocity, 0.0f, 0.0f);
    } else if (ball_pos.x + ball_radius > bottom_right.x) {
        handle_goal(RightSide);
        Vec2_set(&ball_velocity, 0.0f, 0.0f);
    }
}
