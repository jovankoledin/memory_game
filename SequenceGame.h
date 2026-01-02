#ifndef SEQUENCE_GAME_H
#define SEQUENCE_GAME_H

#include "raylib.h"
#include <vector>
#include <random>

enum SequenceState {
    SEQ_MENU,
    SEQ_MEMORIZE, // Numbers are visible
    SEQ_PLAYING,  // Numbers are hidden, player clicking
    SEQ_GAMEOVER
};

struct SequenceTile {
    Rectangle rect;
    int value;      // 1, 2, 3...
    bool visible;   // Is the number showing?
    bool clicked;   // Has this been successfully clicked?
    int gridIndex;  // To ensure no overlap
};

class SequenceGame {
public:
    void Init();
    void Update();
    void Draw();
    bool IsActive();
    void ReturnToMenu();

private:
    // Game State
    SequenceState state;
    bool requestExit;
    
    // Gameplay Data
    std::vector<SequenceTile> tiles;
    int currentLevel;   // Level 1 = 4 numbers, Level 2 = 5, etc.
    int nextExpected;   // The next number the player must click
    int score;
    int maxScore;       // High score for this session

    // RNG
    std::mt19937 rng;

    // Helpers
    void StartLevel(int numCount);
    void HandleInput();
    void DrawTiles();
};

#endif