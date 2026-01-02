#ifndef KILLER_SUDOKU_H
#define KILLER_SUDOKU_H

#include "raylib.h"
#include <vector>
#include <string>
#include <random>

// Shared Enums (You can move these to a common.h later if you want)
enum SudokuDifficulty {
    S_MEDIUM, // Smaller cages, maybe a revealed number
    S_HARD    // Larger cages, empty board
};

struct SudokuCell {
    int value;          // The correct solution value
    int currentInput;   // What the player typed (0 if empty)
    int cageID;         // ID to group cells
    bool isFixed;       // If true, player cannot change (rare in Killer, but good to have)
    bool isError;       // For visual feedback
};

struct Cage {
    int id;
    int targetSum;
    std::vector<int> cellIndices; // Grid indices (0-80) belonging to this cage
    Color color; // Subtle background tint
};

class KillerSudokuGame {
public:
    void Init();
    void StartGame(SudokuDifficulty diff);
    void Update();
    void Draw();
    bool IsActive(); 
    void ReturnToMenu();

    // Helper for main.cpp to get score
    int GetScore() const { return score; }

private:
    // Game State
    SudokuCell grid[81];
    std::vector<Cage> cages;
    int selectedIndex; // -1 if nothing selected
    int score;
    int timer;
    double timeAccumulator;
    bool isComplete;
    bool isActive;
    std::mt19937 rng;

    
    // Generation Helpers
    void ClearGrid();
    bool GenerateFullSolution(int index);
    bool IsSafe(int index, int num);
    void GenerateCages(SudokuDifficulty diff);
    
    // Gameplay Helpers
    void CheckErrors();
    bool CheckWinCondition();
    void DrawBoard();
    void DrawInputPad();
};

#endif