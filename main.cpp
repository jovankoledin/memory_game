#include "raylib.h"
#include <cstdio> 

// Game Headers
#include "KillerSudoku.h"
#include "MemoryGame.h"
#include <emscripten/emscripten.h>

// --- Constants ---
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

// --- Enums ---
enum AppState {
    APP_MAIN_MENU,
    APP_MEMORY_GAME,
    APP_SUDOKU_GAME
};

// --- Globals ---
AppState appState = APP_MAIN_MENU;

// Game Instances
KillerSudokuGame sudokuGame;
MemoryGame memoryGame;

void UpdateDrawFrame(void);

// --- Main ---
int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib Game Arcade");
    
    // Initialize Games
    sudokuGame.Init();
    memoryGame.Init();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }
#endif

    CloseWindow();
    return 0;
}

// --- MAIN LOOP ---
void UpdateDrawFrame() {
    switch(appState) {

        case APP_MAIN_MENU: {
            BeginDrawing();
            ClearBackground(RAYWHITE);
            
            DrawText("ARCADE MENU", SCREEN_WIDTH/2 - MeasureText("ARCADE MENU", 50)/2, 100, 50, DARKGRAY);
            
            Rectangle btnMem = { (float)SCREEN_WIDTH/2 - 120, 250, 240, 60 };
            Rectangle btnSud = { (float)SCREEN_WIDTH/2 - 120, 340, 240, 60 };
            
            Vector2 mousePos = GetMousePosition();
            bool click = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
            
            DrawRectangleRec(btnMem, CheckCollisionPointRec(mousePos, btnMem) ? SKYBLUE : LIGHTGRAY);
            DrawRectangleLinesEx(btnMem, 2, DARKGRAY);
            DrawText("Memory Game", btnMem.x + 40, btnMem.y + 20, 20, DARKGRAY);
            
            DrawRectangleRec(btnSud, CheckCollisionPointRec(mousePos, btnSud) ? GOLD : LIGHTGRAY);
            DrawRectangleLinesEx(btnSud, 2, DARKGRAY);
            DrawText("Killer Sudoku", btnSud.x + 40, btnSud.y + 20, 20, DARKGRAY);
            
            if (click) {
                if (CheckCollisionPointRec(mousePos, btnMem)) {
                    appState = APP_MEMORY_GAME;
                    memoryGame.Init();
                } else if (CheckCollisionPointRec(mousePos, btnSud)) {
                    appState = APP_SUDOKU_GAME;
                    sudokuGame.StartGame(S_MEDIUM); 
                }
            }
            EndDrawing();
        }
        break;

        case APP_MEMORY_GAME: {
            memoryGame.Update();
            
            BeginDrawing();
            ClearBackground(RAYWHITE);
            memoryGame.Draw();
            EndDrawing();
            
            if (!memoryGame.IsActive()) {
                appState = APP_MAIN_MENU;
                memoryGame.ReturnToMenu();
            }
        }
        break;

        case APP_SUDOKU_GAME: {
            sudokuGame.Update();
            
            BeginDrawing();
            ClearBackground(RAYWHITE);
            sudokuGame.Draw();
            EndDrawing();
            
            if (!sudokuGame.IsActive()) {
                appState = APP_MAIN_MENU;
            }
        }
        break;
    }
}
