#ifndef MEMORY_GAME_H
#define MEMORY_GAME_H

#include "raylib.h"
#include <vector>
#include <string>

// --- Enums & Structs specific to Memory Game ---
enum MemoryDifficulty {
    DIFF_MEDIUM, 
    DIFF_HARD    
};

enum MemoryGameState {
    MEM_MENU,
    MEM_PLAYING,
    MEM_WAITING, 
    MEM_GAMEOVER,
    MEM_HELP
};

struct Card {
    Rectangle rect;
    Color color;
    int id;       
    int gridIndex; 
    bool flipped; 
    bool matched;
    bool active;  
    KeyboardKey assignedKey; 
    char keyLabel[2];
    float flipProgress; 
};

struct KeyDefinition {
    KeyboardKey key;
    char label[2]; 
};

class MemoryGame {
public:
    void Init();
    void StartGame(MemoryDifficulty diff);
    void Update();
    void Draw();
    bool IsActive();
    void ReturnToMenu();

private:
    // Game State
    std::vector<Card> cards;
    MemoryGameState state;
    MemoryDifficulty currentDifficulty;
    
    // Logic Pointers
    Card* firstSelection;
    Card* secondSelection;
    
    // Stats
    double waitTimer;
    int matchesFound;
    int moves;
    int errors;
    int totalPairs;
    int finalScore; 
    bool requestExit; // NEW: Flag to signal main.cpp to change AppState
    int gameTime;           
    double timeAccumulator;
    std::vector<bool> cardSeen; 

    // Internal Helpers
    void DrawCard(const Card& card);
    std::vector<KeyDefinition> GetKeyPool();
    void CheckMatch();
    void HandleMenuInput();
};

#endif