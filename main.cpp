#include "raylib.h"
#include <cstdio> 

// Game Headers
#include "KillerSudoku.h"
#include "MemoryGame.h"
#include "SequenceGame.h"
#include <emscripten/emscripten.h>

// --- Constants ---
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

// --- Enums ---
enum AppState {
    APP_MAIN_MENU,
    APP_MEMORY_GAME,
    APP_SUDOKU_GAME,
    APP_SEQUENCE_GAME
};

// --- Globals ---
AppState appState = APP_MAIN_MENU;

// Game Instances
KillerSudokuGame sudokuGame;
MemoryGame memoryGame;
SequenceGame sequenceGame;

void UpdateDrawFrame(void);

// --- Main ---
int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib Game Arcade");
    
    // Initialize Games
    sudokuGame.Init();
    memoryGame.Init();
    sequenceGame.Init();

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
            
            DrawText("ARCADE MENU", SCREEN_WIDTH/2 - MeasureText("ARCADE MENU", 50)/2, 80, 50, DARKGRAY);
            
            // Adjusted positions for 3 buttons
            float centerX = (float)SCREEN_WIDTH/2 - 120;
            Rectangle btnMem = { centerX, 200, 240, 60 };
            Rectangle btnSud = { centerX, 280, 240, 60 };
            Rectangle btnSeq = { centerX, 360, 240, 60 }; // <--- NEW BUTTON
            
            Vector2 mousePos = GetMousePosition();
            bool click = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
            
            // Memory Game Button
            DrawRectangleRec(btnMem, CheckCollisionPointRec(mousePos, btnMem) ? SKYBLUE : LIGHTGRAY);
            DrawRectangleLinesEx(btnMem, 2, DARKGRAY);
            DrawText("Memory Cards", btnMem.x + 40, btnMem.y + 20, 20, DARKGRAY);
            
            // Sudoku Button
            DrawRectangleRec(btnSud, CheckCollisionPointRec(mousePos, btnSud) ? GOLD : LIGHTGRAY);
            DrawRectangleLinesEx(btnSud, 2, DARKGRAY);
            DrawText("Killer Sudoku", btnSud.x + 40, btnSud.y + 20, 20, DARKGRAY);

            // Sequence Button (NEW)
            DrawRectangleRec(btnSeq, CheckCollisionPointRec(mousePos, btnSeq) ? ORANGE : LIGHTGRAY);
            DrawRectangleLinesEx(btnSeq, 2, DARKGRAY);
            DrawText("Sequence Memory", btnSeq.x + 30, btnSeq.y + 20, 20, DARKGRAY);
            
            if (click) {
                if (CheckCollisionPointRec(mousePos, btnMem)) {
                    appState = APP_MEMORY_GAME;
                    memoryGame.Init();
                } else if (CheckCollisionPointRec(mousePos, btnSud)) {
                    appState = APP_SUDOKU_GAME;
                    sudokuGame.StartGame(S_MEDIUM); 
                } else if (CheckCollisionPointRec(mousePos, btnSeq)) {
                    appState = APP_SEQUENCE_GAME;
                    sequenceGame.Init(); // <--- NEW
                }
            }
            EndDrawing();
        }
        break;

        // ... Keep APP_MEMORY_GAME and APP_SUDOKU_GAME as they were ...
        case APP_MEMORY_GAME: {
             // (Existing Memory Game Logic)
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
             // (Existing Sudoku Logic)
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

        // NEW CASE
        case APP_SEQUENCE_GAME: {
            sequenceGame.Update();
            
            BeginDrawing();
            // SequenceGame::Draw() handles ClearBackground, but good practice to have it here too if needed
            sequenceGame.Draw();
            EndDrawing();
            
            if (!sequenceGame.IsActive()) {
                appState = APP_MAIN_MENU;
                sequenceGame.ReturnToMenu();
            }
        }
        break;
    }
}
