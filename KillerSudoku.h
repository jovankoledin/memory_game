#ifndef KILLER_SUDOKU_H
#define KILLER_SUDOKU_H

#include "raylib.h"
#include <vector>
#include <random>

enum SudokuDifficulty {
    S_MEDIUM, // ~40 numbers revealed
    S_HARD    // ~25 numbers revealed
};

struct SudokuCell {
    int value;          // The correct hidden solution
    int currentInput;   // What the player typed
    bool isFixed;       // If true, this was a starting clue
    bool isError;       // For visual feedback
};

class KillerSudokuGame {
public:
    void Init();
    void StartGame(SudokuDifficulty diff);
    void Update();
    void Draw();
    bool IsActive(); 
    void ReturnToMenu();
    int GetScore() const { return score; }

private:
    SudokuCell grid[81];
    int selectedIndex; 
    int score;
    int timer;
    double timeAccumulator;
    bool isComplete;
    bool isActive;
    std::mt19937 rng;

    void ClearGrid();
    bool GenerateFullSolution(int index);
    bool IsSafe(int index, int num);
    void CreatePuzzle(SudokuDifficulty diff); // Replaces GenerateCages
    
    void CheckErrors();
    bool CheckWinCondition();
    void DrawBoard();
};

#endif