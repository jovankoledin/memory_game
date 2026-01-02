#include "KillerSudoku.h"
#include <algorithm>
#include <random>
#include <set>
#include <cstdio>
#include <cmath>
#include "js_interop.h"
#include <iostream>

// Constants
const int CELL_SIZE = 50;
const int GRID_OFFSET_X = 175; // Center horizontally(ish)
const int GRID_OFFSET_Y = 50;

// Pastel colors for cages
const Color CAGE_COLORS[] = {
    {255, 230, 230, 100}, {230, 255, 230, 100}, {230, 230, 255, 100},
    {255, 255, 230, 100}, {255, 230, 255, 100}, {230, 255, 255, 100}
};

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
    // seed RNG once
    std::random_device rd;
    rng.seed(rd());

    std::cout << "--- START GAME 1: Before Diagonal Fill ---\n"; // Use the provided JsLog or a simple printf

    // 1. Generate a valid full Sudoku grid
    ClearGrid();

    std::cout << "--- START GAME 2: Before Full Solution Generation ---\n";

    // Fill diagonal 3x3 boxes first (independent of each other) to speed up solver
    std::mt19937 g(rd());
    
    for (int box = 0; box < 9; box += 4) {
        std::vector<int> nums = {1,2,3,4,5,6,7,8,9};
        std::shuffle(nums.begin(), nums.end(), g);
        int startRow = (box / 3) * 3;
        int startCol = (box % 3) * 3;
        int n = 0;
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                grid[(startRow + r) * 9 + (startCol + c)].value = nums[n++];
            }
        }
    }
    
    GenerateFullSolution(0);
    std::cout << "--- START GAME 3: Before Cage Generation ---\n"; // If the crash happens before this, look at GenerateFullSolution

    // 2. Generate Cages based on the solution
    GenerateCages(diff);
    std::cout << "--- START GAME 4: Before Final Clear/Setup ---\n"; // If the crash happens here, look at GenerateCages

    // 3. Clear inputs for the player
    for (int i = 0; i < 81; i++) {
        grid[i].currentInput = 0;
        grid[i].isError = false;
        // In Killer Sudoku, usually NO numbers are given, only sums.
        // For Medium, we might reveal a few random ones.
        grid[i].isFixed = false;
    }

    if (diff == S_MEDIUM) {
        // Reveal 10 random cells for medium
        for(int k=0; k<10; k++) {
            int idx = g() % 81;
            grid[idx].currentInput = grid[idx].value;
            grid[idx].isFixed = true;
        }
    }
}

void KillerSudokuGame::ClearGrid() {
    for (int i = 0; i < 81; i++) {
        grid[i].value = 0;
        grid[i].currentInput = 0;
        grid[i].cageID = -1;
        grid[i].isFixed = false;
        grid[i].isError = false;
    }
    cages.clear();
}

bool KillerSudokuGame::IsSafe(int index, int num) {
    int row = index / 9;
    int col = index % 9;
    
    // Check Row & Col
    for (int i = 0; i < 9; i++) {
        if (grid[row * 9 + i].value == num) return false;
        if (grid[i * 9 + col].value == num) return false;
    }
    
    // Check 3x3 Box
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

    // Skip if already filled (by diagonal initialization)
    if (grid[index].value != 0) return GenerateFullSolution(index + 1);

    std::vector<int> nums = {1,2,3,4,5,6,7,8,9};
    std::shuffle(nums.begin(), nums.end(), rng); // <<-- use member rng, NOT a local one

    for (int num : nums) {
        if (IsSafe(index, num)) {
            grid[index].value = num;
            if (GenerateFullSolution(index + 1)) return true;
            grid[index].value = 0;
        }
    }
    return false;
}

void KillerSudokuGame::GenerateCages(SudokuDifficulty diff)
{
    int maxCageSize = (diff == S_MEDIUM) ? 4 : 6;
    cages.clear();

    for (int i = 0; i < 81; ++i)
        grid[i].cageID = -1;

    std::vector<int> availableCells(81);
    std::iota(availableCells.begin(), availableCells.end(), 0);
    std::shuffle(availableCells.begin(), availableCells.end(), rng);

    const int dirs[4] = {-9, 9, -1, 1};
    int nextCageID = 0;

    // -------------------------
    // Phase 1: initial cages
    // -------------------------
    while (!availableCells.empty()) {
        int startIdx = availableCells.back();
        availableCells.pop_back();

        if (grid[startIdx].cageID != -1)
            continue;

        int c = startIdx % 9;

        std::vector<int> freeNeighbors;
        for (int d : dirs) {
            int n = startIdx + d;
            if (n < 0 || n >= 81) continue;
            if (c == 0 && d == -1) continue;
            if (c == 8 && d == 1)  continue;
            if (grid[n].cageID == -1)
                freeNeighbors.push_back(n);
        }

        // Isolated → merge immediately
        if (freeNeighbors.empty()) {
            std::vector<int> neighborCages;
            for (int d : dirs) {
                int n = startIdx + d;
                if (n < 0 || n >= 81) continue;
                if (c == 0 && d == -1) continue;
                if (c == 8 && d == 1)  continue;
                if (grid[n].cageID != -1)
                    neighborCages.push_back(grid[n].cageID);
            }

            if (neighborCages.empty())
                std::abort();

            int mergeID = neighborCages[rng() % neighborCages.size()];
            auto it = std::find_if(cages.begin(), cages.end(),
                [&](const Cage& c) { return c.id == mergeID; });

            it->cellIndices.push_back(startIdx);
            it->targetSum += grid[startIdx].value;
            grid[startIdx].cageID = mergeID;
            continue;
        }

        // Start new cage (forced ≥ 2)
        Cage cage;
        cage.id = nextCageID++;
        cage.color = CAGE_COLORS[cage.id % 6];
        cage.targetSum = 0;

        std::vector<int> cells;
        cells.push_back(startIdx);
        grid[startIdx].cageID = cage.id;
        cage.targetSum += grid[startIdx].value;

        int first = freeNeighbors[rng() % freeNeighbors.size()];
        cells.push_back(first);
        grid[first].cageID = cage.id;
        cage.targetSum += grid[first].value;

        availableCells.erase(
            std::remove(availableCells.begin(), availableCells.end(), first),
            availableCells.end()
        );

        int targetSize = 2 + rng() % (maxCageSize - 1);

        while ((int)cells.size() < targetSize) {
            std::vector<int> frontier;
            for (int cell : cells) {
                int cc = cell % 9;
                for (int d : dirs) {
                    int n = cell + d;
                    if (n < 0 || n >= 81) continue;
                    if (cc == 0 && d == -1) continue;
                    if (cc == 8 && d == 1)  continue;
                    if (grid[n].cageID == -1)
                        frontier.push_back(n);
                }
            }

            if (frontier.empty())
                break;

            frontier.erase(std::unique(frontier.begin(), frontier.end()), frontier.end());
            int next = frontier[rng() % frontier.size()];

            cells.push_back(next);
            grid[next].cageID = cage.id;
            cage.targetSum += grid[next].value;

            availableCells.erase(
                std::remove(availableCells.begin(), availableCells.end(), next),
                availableCells.end()
            );
        }

        cage.cellIndices = std::move(cells);
        cages.push_back(std::move(cage));
    }

    // -------------------------
    // Phase 2: HARD singleton merge
    // -------------------------
    bool changed = true;
    while (changed) {
        changed = false;

        for (auto it = cages.begin(); it != cages.end(); ) {
            if (it->cellIndices.size() != 1) {
                ++it;
                continue;
            }

            int cell = it->cellIndices[0];
            int c = cell % 9;

            int bestID = -1;
            size_t bestSize = SIZE_MAX;

            for (int d : dirs) {
                int n = cell + d;
                if (n < 0 || n >= 81) continue;
                if (c == 0 && d == -1) continue;
                if (c == 8 && d == 1)  continue;

                int cid = grid[n].cageID;
                if (cid == it->id || cid == -1) continue;

                auto jt = std::find_if(cages.begin(), cages.end(),
                    [&](const Cage& c) { return c.id == cid; });

                if (jt != cages.end() && jt->cellIndices.size() < bestSize) {
                    bestSize = jt->cellIndices.size();
                    bestID = cid;
                }
            }

            if (bestID == -1)
                std::abort();

            auto target = std::find_if(cages.begin(), cages.end(),
                [&](const Cage& c) { return c.id == bestID; });

            target->cellIndices.push_back(cell);
            target->targetSum += grid[cell].value;
            grid[cell].cageID = bestID;

            it = cages.erase(it);
            changed = true;
        }
    }

#ifndef NDEBUG
    for (const auto& cage : cages) {
        if (cage.cellIndices.size() < 2) {
            std::cerr << "BUG: single-cell cage survived\n";
            std::abort();
        }
    }
#endif
}

void KillerSudokuGame::Update() {
    if (!isActive) return;
    if (isComplete) return; // Stop input if won

    // Timer
    timeAccumulator += GetFrameTime();
    if (timeAccumulator >= 1.0) {
        timer++;
        timeAccumulator -= 1.0;
    }

    // Input Handling
    Vector2 mousePos = GetMousePosition();
    
    // Grid Selection
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        selectedIndex = -1;
        int gridX = (mousePos.x - GRID_OFFSET_X) / CELL_SIZE;
        int gridY = (mousePos.y - GRID_OFFSET_Y) / CELL_SIZE;
        
        if (gridX >= 0 && gridX < 9 && gridY >= 0 && gridY < 9) {
            int idx = gridY * 9 + gridX;
            if (!grid[idx].isFixed) {
                selectedIndex = idx;
            }
        }
    }

    // Keyboard Input
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
                score = (10000 / (timer + 1)); // Simple score based on time

                // Save result once (High-is-better -> sortOrder = 1)
                SaveScoreToBrowser(score, 1);
            }
        }

        
        if (key == KEY_BACKSPACE || key == KEY_DELETE) {
            grid[selectedIndex].currentInput = 0;
            grid[selectedIndex].isError = false;
        }
        
        // Arrows navigation
        if (key == KEY_UP && selectedIndex >= 9) selectedIndex -= 9;
        if (key == KEY_DOWN && selectedIndex < 72) selectedIndex += 9;
        if (key == KEY_LEFT && selectedIndex % 9 != 0) selectedIndex -= 1;
        if (key == KEY_RIGHT && selectedIndex % 9 != 8) selectedIndex += 1;
    }
}

void KillerSudokuGame::CheckErrors() {
    // Basic standard sudoku check (duplicates in row/col/box)
    // In a real game, you might not show errors immediately, but for casual play it's nice.
    for (int i = 0; i < 81; i++) grid[i].isError = false;

    // We only check against the pre-generated solution for simplicity here
    // But strictly we should check game rules. 
    for (int i = 0; i < 81; i++) {
        if (grid[i].currentInput != 0 && grid[i].currentInput != grid[i].value) {
            grid[i].isError = true;
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
    
    // HUD
    DrawText(TextFormat("Time: %02i:%02i", timer/60, timer%60), 20, 20, 20, DARKGRAY);
    
    if (isComplete) {
        DrawText("PUZZLE SOLVED!", 300, 10, 30, GOLD);
        DrawText(TextFormat("Score: %i", score), 320, 45, 20, DARKGREEN);
    }
    
    // Back Button
    Rectangle btnBack = { 20, 550, 80, 30 };
    DrawRectangleRec(btnBack, LIGHTGRAY);
    DrawRectangleLinesEx(btnBack, 1, DARKGRAY);
    DrawText("MENU", 35, 558, 16, DARKGRAY);
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), btnBack)) {
        ReturnToMenu();
    }
}

void KillerSudokuGame::DrawBoard() {
    // 1. Draw Cages (Backgrounds)
    for (int i = 0; i < 81; i++) {
        int r = i / 9;
        int c = i % 9;
        int x = GRID_OFFSET_X + c * CELL_SIZE;
        int y = GRID_OFFSET_Y + r * CELL_SIZE;
        
        // Find cage
        for (const auto& cage : cages) {
            if (cage.id == grid[i].cageID) {
                DrawRectangle(x, y, CELL_SIZE, CELL_SIZE, cage.color);
                break;
            }
        }
    }

    // 2. Draw Cage Targets (Top left of the first cell in the cage usually, 
    // or just the top-most left-most cell of the cage)
    for (const auto& cage : cages) {
        // Find top-left most cell in cage
        int minIdx = 999;
        for (int idx : cage.cellIndices) {
            if (idx < minIdx) minIdx = idx;
        }
        int r = minIdx / 9;
        int c = minIdx % 9;
        DrawText(TextFormat("%i", cage.targetSum), 
                 GRID_OFFSET_X + c * CELL_SIZE + 2, 
                 GRID_OFFSET_Y + r * CELL_SIZE + 2, 
                 10, BLACK);
    }

    // 3. Draw Grid Lines
    for (int i = 0; i <= 9; i++) {
        int thickness = (i % 3 == 0) ? 3 : 1;
        DrawLineEx(
            {(float)GRID_OFFSET_X + i*CELL_SIZE, (float)GRID_OFFSET_Y}, 
            {(float)GRID_OFFSET_X + i*CELL_SIZE, (float)GRID_OFFSET_Y + 9*CELL_SIZE}, 
            thickness, BLACK
        );
        DrawLineEx(
            {(float)GRID_OFFSET_X, (float)GRID_OFFSET_Y + i*CELL_SIZE}, 
            {(float)GRID_OFFSET_X + 9*CELL_SIZE, (float)GRID_OFFSET_Y + i*CELL_SIZE}, 
            thickness, BLACK
        );
    }
    
    // 4. Draw Cage Borders (Dotted lines usually, but thin dashed lines here)
    // This is tricky in immediate mode, simplified: 
    // We draw lines between cells if their Cage IDs are different.
    for (int i = 0; i < 81; i++) {
        int r = i / 9;
        int c = i % 9;
        int x = GRID_OFFSET_X + c * CELL_SIZE;
        int y = GRID_OFFSET_Y + r * CELL_SIZE;
        
        // Check Right
        if (c < 8) {
            if (grid[i].cageID != grid[i+1].cageID) {
                DrawLine(x+CELL_SIZE, y, x+CELL_SIZE, y+CELL_SIZE, DARKGRAY);
            }
        }
        // Check Down
        if (r < 8) {
            if (grid[i].cageID != grid[i+9].cageID) {
                DrawLine(x, y+CELL_SIZE, x+CELL_SIZE, y+CELL_SIZE, DARKGRAY);
            }
        }
    }

    // 5. Draw Numbers & Selection
    for (int i = 0; i < 81; i++) {
        int r = i / 9;
        int c = i % 9;
        int x = GRID_OFFSET_X + c * CELL_SIZE;
        int y = GRID_OFFSET_Y + r * CELL_SIZE;

        // Selection Highlight
        if (i == selectedIndex) {
            DrawRectangleLinesEx({(float)x+2,(float)y+2,(float)CELL_SIZE-4,(float)CELL_SIZE-4}, 2, SKYBLUE);
        }

        if (grid[i].currentInput != 0) {
            Color numColor = grid[i].isFixed ? BLACK : (grid[i].isError ? RED : DARKBLUE);
            const char* txt = TextFormat("%i", grid[i].currentInput);
            int txtW = MeasureText(txt, 30);
            DrawText(txt, x + (CELL_SIZE - txtW)/2, y + 10, 30, numColor);
        }
    }
}

void KillerSudokuGame::ReturnToMenu() {
    isActive = false;
}

bool KillerSudokuGame::IsActive() {
    return isActive;
}