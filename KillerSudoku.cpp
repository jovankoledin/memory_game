#include "KillerSudoku.h"
#include <algorithm>
#include <iostream>
#include "js_interop.h" // Assuming this handles SaveScoreToBrowser

const int CELL_SIZE = 50;
const int GRID_OFFSET_X = 175;
const int GRID_OFFSET_Y = 50;

void KillerSudokuGame::Init() {
    isActive = false;
    selectedIndex = -1;
}

void KillerSudokuGame::StartGame(SudokuDifficulty diff) {
    isActive = true;
    isComplete = false;
    score = 0;
    timer = 0;
    timeAccumulator = 0.0;
    selectedIndex = -1;

    std::random_device rd;
    rng.seed(rd());

    ClearGrid();

    // 1. Fill diagonal 3x3 boxes to speed up generation
    for (int box = 0; box < 9; box += 4) {
        std::vector<int> nums = {1,2,3,4,5,6,7,8,9};
        std::shuffle(nums.begin(), nums.end(), rng);
        int startRow = (box / 3) * 3;
        int startCol = (box % 3) * 3;
        int n = 0;
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                grid[(startRow + r) * 9 + (startCol + c)].value = nums[n++];
            }
        }
    }

    // 2. Solve the rest of the board
    GenerateFullSolution(0);

    // 3. Create the puzzle by hiding numbers
    CreatePuzzle(diff);
}

void KillerSudokuGame::ClearGrid() {
    for (int i = 0; i < 81; i++) {
        grid[i].value = 0;
        grid[i].currentInput = 0;
        grid[i].isFixed = false;
        grid[i].isError = false;
    }
}

bool KillerSudokuGame::IsSafe(int index, int num) {
    int row = index / 9;
    int col = index % 9;
    for (int i = 0; i < 9; i++) {
        if (grid[row * 9 + i].value == num) return false;
        if (grid[i * 9 + col].value == num) return false;
    }
    int startRow = (row / 3) * 3;
    int startCol = (col / 3) * 3;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (grid[(startRow + i) * 9 + (startCol + j)].value == num) return false;
        }
    }
    return true;
}

bool KillerSudokuGame::GenerateFullSolution(int index) {
    if (index == 81) return true;
    if (grid[index].value != 0) return GenerateFullSolution(index + 1);

    std::vector<int> nums = {1,2,3,4,5,6,7,8,9};
    std::shuffle(nums.begin(), nums.end(), rng);

    for (int num : nums) {
        if (IsSafe(index, num)) {
            grid[index].value = num;
            if (GenerateFullSolution(index + 1)) return true;
            grid[index].value = 0;
        }
    }
    return false;
}

void KillerSudokuGame::CreatePuzzle(SudokuDifficulty diff) {
    // Determine how many clues to keep
    int cluesToKeep = (diff == S_MEDIUM) ? 35 : 24;
    
    // Create a list of all indices and shuffle them
    std::vector<int> indices(81);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), rng);

    // Set all cells as playable first
    for(int i=0; i<81; i++) {
        grid[i].currentInput = 0;
        grid[i].isFixed = false;
    }

    // Assign fixed clues
    for (int i = 0; i < cluesToKeep; i++) {
        int idx = indices[i];
        grid[idx].currentInput = grid[idx].value;
        grid[idx].isFixed = true;
    }
}

void KillerSudokuGame::Update() {
    if (!isActive || isComplete) return;

    timeAccumulator += GetFrameTime();
    if (timeAccumulator >= 1.0) {
        timer++;
        timeAccumulator -= 1.0;
    }

    Vector2 mousePos = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        selectedIndex = -1;
        int gridX = (mousePos.x - GRID_OFFSET_X) / CELL_SIZE;
        int gridY = (mousePos.y - GRID_OFFSET_Y) / CELL_SIZE;
        if (gridX >= 0 && gridX < 9 && gridY >= 0 && gridY < 9) {
            int idx = gridY * 9 + gridX;
            if (!grid[idx].isFixed) selectedIndex = idx;
        }
    }

    int key = GetKeyPressed();
    if (selectedIndex != -1) {
        int num = -1;
        if (key >= KEY_ONE && key <= KEY_NINE) num = key - KEY_ONE + 1;
        if (key >= KEY_KP_1 && key <= KEY_KP_9) num = key - KEY_KP_1 + 1;
        
        if (num != -1) {
            grid[selectedIndex].currentInput = num;
            CheckErrors();
            if (CheckWinCondition()) {
                isComplete = true;
                score = (10000 / (timer + 1));
                SaveScoreToBrowser(score, 1);
            }
        }
        if (key == KEY_BACKSPACE || key == KEY_DELETE) {
            grid[selectedIndex].currentInput = 0;
            grid[selectedIndex].isError = false;
        }
    }
}

void KillerSudokuGame::CheckErrors() {
    for (int i = 0; i < 81; i++) {
        if (grid[i].currentInput != 0 && grid[i].currentInput != grid[i].value) {
            grid[i].isError = true;
        } else {
            grid[i].isError = false;
        }
    }
}

bool KillerSudokuGame::CheckWinCondition() {
    for (int i = 0; i < 81; i++) {
        if (grid[i].currentInput != grid[i].value) return false;
    }
    return true;
}

void KillerSudokuGame::Draw() {
    if (!isActive) return;

    DrawBoard();
    DrawText(TextFormat("Time: %02i:%02i", timer/60, timer%60), 20, 20, 20, DARKGRAY);
    
    if (isComplete) {
        DrawText("SUDOKU SOLVED!", 300, 10, 30, GOLD);
    }
    
    Rectangle btnBack = { 20, 550, 80, 30 };
    DrawRectangleRec(btnBack, LIGHTGRAY);
    DrawText("MENU", 35, 558, 16, DARKGRAY);
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), btnBack)) {
        ReturnToMenu();
    }
}

void KillerSudokuGame::DrawBoard() {
    // Background
    DrawRectangle(GRID_OFFSET_X, GRID_OFFSET_Y, 9 * CELL_SIZE, 9 * CELL_SIZE, RAYWHITE);

    // Grid Lines
    for (int i = 0; i <= 9; i++) {
        int thickness = (i % 3 == 0) ? 3 : 1;
        DrawLineEx({(float)GRID_OFFSET_X + i*CELL_SIZE, (float)GRID_OFFSET_Y}, 
                   {(float)GRID_OFFSET_X + i*CELL_SIZE, (float)GRID_OFFSET_Y + 9*CELL_SIZE}, thickness, BLACK);
        DrawLineEx({(float)GRID_OFFSET_X, (float)GRID_OFFSET_Y + i*CELL_SIZE}, 
                   {(float)GRID_OFFSET_X + 9*CELL_SIZE, (float)GRID_OFFSET_Y + i*CELL_SIZE}, thickness, BLACK);
    }

    // Numbers
    for (int i = 0; i < 81; i++) {
        int r = i / 9, c = i % 9;
        int x = GRID_OFFSET_X + c * CELL_SIZE;
        int y = GRID_OFFSET_Y + r * CELL_SIZE;

        if (i == selectedIndex) {
            DrawRectangle(x + 2, y + 2, CELL_SIZE - 4, CELL_SIZE - 4, LIGHTGRAY);
        }

        if (grid[i].currentInput != 0) {
            Color numColor = grid[i].isFixed ? BLACK : (grid[i].isError ? RED : DARKBLUE);
            const char* txt = TextFormat("%i", grid[i].currentInput);
            int txtW = MeasureText(txt, 30);
            DrawText(txt, x + (CELL_SIZE - txtW)/2, y + 10, 30, numColor);
        }
    }
}

void KillerSudokuGame::ReturnToMenu() { isActive = false; }
bool KillerSudokuGame::IsActive() { return isActive; }