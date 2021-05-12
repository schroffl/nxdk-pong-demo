#include "vec2.h"

typedef enum {
    PlayerLeftBall,
    PlayerRightBall,
    Playing,
    PlayerLeftWon,
    PlayerRightWon,
} State;

typedef struct {
    unsigned int player_left_points;
    unsigned int player_right_points;

    float player_left_pos;
    float player_right_pos;

    Vec2 ball_pos;
    State state;
} GameState;

GameState game_state = {
    .player_left_points = 0,
    .player_right_points = 0,
    .player_left_pos = 0,
    .player_right_pos = 0,
    .ball_pos = {0.0f, 0.0f},
    .state = PlayerLeftBall,
};

enum GoalSide {
    LeftSide,
    RightSide,
};

void handle_goal(enum GoalSide goal_side) {
    switch (goal_side) {
        case LeftSide:
            game_state.player_right_points += 1;
            game_state.state = PlayerLeftBall;
            break;

        case RightSide:
            game_state.player_left_points += 1;
            game_state.state = PlayerRightBall;
            break;
    }

    Vec2_set(&game_state.ball_pos, 0.0f, 0.0f);
    game_state.player_left_pos = 0.0f;
    game_state.player_right_pos = 0.0f;

    if (game_state.player_right_points >= 3) {
        game_state.state = PlayerRightWon;
    } else if (game_state.player_left_points >= 3) {
        game_state.state = PlayerLeftWon;
    }
}

void restart_game() {
    game_state.player_left_points = 0;
    game_state.player_right_points = 0;

    game_state.player_left_pos = 0;
    game_state.player_right_pos = 0;

    Vec2_set(&game_state.ball_pos, 0.0f, 0.0f);
    game_state.state = PlayerLeftBall;
}
