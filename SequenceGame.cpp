#include "SequenceGame.h"
#include <algorithm>
#include <string>
#include <cstdio> // For TextFormat
#include "js_interop.h" // To save scores

// Configuration
const int SEQ_GRID_COLS = 8;
const int SEQ_GRID_ROWS = 5;
const int SEQ_TILE_SIZE = 80;
const int SEQ_SPACING = 10;
const int SEQ_OFFSET_X = 40; // Adjust to center based on screen 800
const int SEQ_OFFSET_Y = 100; // Adjust to center based on screen 600

void SequenceGame::Init() {
    state = SEQ_MENU;
    requestExit = false;
    currentLevel = 4; // Start with 4 numbers
    score = 0;
    maxScore = 0;
    
    // Seed RNG
    std::random_device rd;
    rng.seed(rd());
}

bool SequenceGame::IsActive() {
    return !requestExit;
}

void SequenceGame::ReturnToMenu() {
    state = SEQ_MENU;
    requestExit = false;
}

void SequenceGame::StartLevel(int numCount) {
    tiles.clear();
    nextExpected = 1;
    
    // 1. Generate all possible grid indices
    std::vector<int> indices;
    for (int i = 0; i < SEQ_GRID_COLS * SEQ_GRID_ROWS; i++) {
        indices.push_back(i);
    }
    
    // 2. Shuffle and pick first 'numCount' positions
    std::shuffle(indices.begin(), indices.end(), rng);
    
    // 3. Create Tiles
    for (int i = 0; i < numCount; i++) {
        SequenceTile t;
        int idx = indices[i];
        int col = idx % SEQ_GRID_COLS;
        int row = idx / SEQ_GRID_COLS;
        
        t.rect = {
            (float)(SEQ_OFFSET_X + col * (SEQ_TILE_SIZE + SEQ_SPACING)),
            (float)(SEQ_OFFSET_Y + row * (SEQ_TILE_SIZE + SEQ_SPACING)),
            (float)SEQ_TILE_SIZE,
            (float)SEQ_TILE_SIZE
        };
        t.value = i + 1;
        t.visible = true;
        t.clicked = false;
        t.gridIndex = idx;
        
        tiles.push_back(t);
    }
    
    state = SEQ_MEMORIZE;
}

void SequenceGame::Update() {
    // Shared Menu Button Logic
    Vector2 mousePos = GetMousePosition();
    bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    Rectangle btnBack = { 20, 20, 80, 30 };
    
    if (clicked && CheckCollisionPointRec(mousePos, btnBack)) {
        if (state == SEQ_MENU) requestExit = true;
        else state = SEQ_MENU;
        return;
    }

    switch(state) {
        case SEQ_MENU: {
            // Start Button
            Rectangle btnStart = { 300, 300, 200, 60 };
            if (clicked && CheckCollisionPointRec(mousePos, btnStart)) {
                score = 0;
                currentLevel = 4; // Reset difficulty
                StartLevel(currentLevel);
            }
        } break;

        case SEQ_MEMORIZE:
        case SEQ_PLAYING: {
            if (clicked) {
                // Check tile clicks
                for (auto &tile : tiles) {
                    if (!tile.clicked && CheckCollisionPointRec(mousePos, tile.rect)) {
                        
                        // Correct click?
                        if (tile.value == nextExpected) {
                            tile.clicked = true;
                            nextExpected++;
                            
                            // GAME RULE: On first click (1), hide all others!
                            if (tile.value == 1) {
                                state = SEQ_PLAYING;
                                for (auto &t : tiles) t.visible = false; 
                            }
                            
                            // Level Complete?
                            if (nextExpected > (int)tiles.size()) {
                                score += currentLevel;
                                if (score > maxScore) maxScore = score;
                                currentLevel++; // Increase difficulty
                                StartLevel(currentLevel);
                            }
                        } 
                        // Wrong click?
                        else {
                            state = SEQ_GAMEOVER;
                            #if defined(PLATFORM_WEB)
                            SaveScoreToBrowser(score, 1); // 1 = High Score is better
                            #endif
                        }
                        break; 
                    }
                }
            }
        } break;

        case SEQ_GAMEOVER: {
            if (clicked && mousePos.y > 200) { // Simple click anywhere to restart
                state = SEQ_MENU;
            }
        } break;
    }
}

void SequenceGame::Draw() {
    ClearBackground(RAYWHITE);
    
    // Draw Back Button
    Rectangle btnBack = { 20, 20, 80, 30 };
    DrawRectangleRec(btnBack, LIGHTGRAY);
    DrawRectangleLinesEx(btnBack, 1, DARKGRAY);
    DrawText("MENU", 35, 28, 10, DARKGRAY);

    switch(state) {
        case SEQ_MENU: {
            DrawText("SEQUENCE MEMORY", 400 - MeasureText("SEQUENCE MEMORY", 40)/2, 150, 40, DARKGRAY);
            DrawText("Click 1, then memorize the rest!", 400 - MeasureText("Click 1, then memorize the rest!", 20)/2, 210, 20, GRAY);
            
            Rectangle btnStart = { 300, 300, 200, 60 };
            DrawRectangleRec(btnStart, SKYBLUE);
            DrawRectangleLinesEx(btnStart, 2, DARKBLUE);
            DrawText("START", 355, 315, 30, WHITE);
        } break;

        case SEQ_MEMORIZE:
        case SEQ_PLAYING: {
            DrawText(TextFormat("Level: %i", currentLevel - 3), 700, 20, 20, DARKGRAY);
            DrawText(TextFormat("Score: %i", score), 700, 45, 20, GOLD);

            for (const auto &tile : tiles) {
                if (tile.clicked) continue; // Don't draw clicked tiles

                DrawRectangleRec(tile.rect, WHITE);
                DrawRectangleLinesEx(tile.rect, 2, DARKGRAY);
                
                // Draw Shadow for depth
                DrawRectangle(tile.rect.x + 4, tile.rect.y + 4, tile.rect.width, tile.rect.height, Fade(BLACK, 0.1f));

                if (state == SEQ_MEMORIZE || tile.visible) {
                    // Show Number
                    const char* text = TextFormat("%i", tile.value);
                    int textW = MeasureText(text, 40);
                    DrawText(text, tile.rect.x + (tile.rect.width - textW)/2, tile.rect.y + 20, 40, DARKBLUE);
                } else {
                    // Show Blank (Hidden)
                    DrawRectangleRec(tile.rect, LIGHTGRAY);
                    DrawRectangleLinesEx(tile.rect, 2, GRAY);
                }
            }
        } break;

        case SEQ_GAMEOVER: {
            DrawText("GAME OVER", 400 - MeasureText("GAME OVER", 60)/2, 200, 60, MAROON);
            
            const char* scoreText = TextFormat("Final Score: %i", score);
            DrawText(scoreText, 400 - MeasureText(scoreText, 30)/2, 280, 30, DARKGRAY);
            
            DrawText("Click to Return", 400 - MeasureText("Click to Return", 20)/2, 350, 20, GRAY);
        } break;
    }
}