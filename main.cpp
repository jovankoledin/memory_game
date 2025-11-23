#include "raylib.h"
#include <vector>
#include <algorithm>
#include <random>
#include <string>
#include <map>
#include <cmath> // Required for fabsf

// Required for Web assembly compilation
#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#if defined(PLATFORM_WEB)
    // Note: We just pass the score. The JS handles the Modal, Validation, and overwrite logic.
    EM_JS(void, SaveScoreToBrowser, (int score), {
        if (typeof window.updateLeaderboard === 'function') {
            window.updateLeaderboard(score);
        }
    });
    // This function is called on startup and game restart to pre-fetch scores for live validation.
    EM_JS(void, RefreshLeaderboard, (), {
        if (typeof window.refreshLeaderboard === 'function') {
            window.refreshLeaderboard();
        }
    });
#else
    void SaveScoreToBrowser(int score) { 
        printf("Game Over! Score: %i\n", score); 
    }
    void RefreshLeaderboard() { }
#endif

// --- Constants & Config ---
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int CARD_SIZE = 90; 
const int CARD_SPACING = 15;
const float FLIP_SPEED = 6.0f; // Speed of the animation (Higher = Faster)

// --- Colors for pairs ---
const Color CARD_COLORS[] = {
    RED, ORANGE, YELLOW, GREEN, SKYBLUE, BLUE, PURPLE, PINK,
    LIME, GOLD, MAROON, DARKBLUE
};

// --- Game State ---
enum GameState {
    STATE_MENU,
    STATE_PLAYING,
    STATE_WAITING, 
    STATE_GAMEOVER
};

enum Difficulty {
    DIFF_MEDIUM, // 4x4
    DIFF_HARD    // 5x5
};

struct Card {
    Rectangle rect;
    Color color;
    int id;       
    int gridIndex; 
    bool flipped; // Logical state (True = user wants to see it)
    bool matched;
    bool active;  
    
    // Animation State
    float flipProgress; // 0.0f (Back) to 1.0f (Front)
};

// --- Globals ---
std::vector<Card> cards;
GameState currentState = STATE_MENU;
Difficulty currentDifficulty = DIFF_MEDIUM;

Card* firstSelection = nullptr;
Card* secondSelection = nullptr;

// Input State
int keyboardCursor = 0; 

// Stats
double waitTimer = 0.0;
int matchesFound = 0;
int moves = 0;
int errors = 0;
int totalPairs = 0;

// Memory Tracking for Errors
std::vector<bool> cardSeen; 

// --- Function Prototypes ---
void InitGame();
void StartGame(Difficulty diff);
void UpdateDrawFrame(void);
void DrawCard(const Card& card);

// --- Main Entry Point ---
int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib Web Memory Game");
    InitGame();

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

// --- Game Logic ---

void InitGame() {
    currentState = STATE_MENU;
}

void StartGame(Difficulty diff) {
    cards.clear();
    matchesFound = 0;
    moves = 0;
    errors = 0;
    firstSelection = nullptr;
    secondSelection = nullptr;
    currentDifficulty = diff;
    currentState = STATE_PLAYING;
    keyboardCursor = 0; 

    int rows = (diff == DIFF_MEDIUM) ? 4 : 5;
    int cols = (diff == DIFF_MEDIUM) ? 4 : 5;
    
    int totalSlots = rows * cols;
    totalPairs = (diff == DIFF_MEDIUM) ? 8 : 12;

    cardSeen.assign(totalSlots, false);

    std::vector<int> ids;
    for (int i = 0; i < totalPairs; i++) {
        ids.push_back(i);
        ids.push_back(i);
    }

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(ids.begin(), ids.end(), g);

    int gridWidth = (cols * CARD_SIZE) + ((cols - 1) * CARD_SPACING);
    int gridHeight = (rows * CARD_SIZE) + ((rows - 1) * CARD_SPACING);
    int offsetX = (SCREEN_WIDTH - gridWidth) / 2;
    int offsetY = (SCREEN_HEIGHT - gridHeight) / 2;

    int idCounter = 0;
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            Card c;
            c.rect = {
                (float)(offsetX + x * (CARD_SIZE + CARD_SPACING)),
                (float)(offsetY + y * (CARD_SIZE + CARD_SPACING)),
                (float)CARD_SIZE,
                (float)CARD_SIZE
            };
            c.gridIndex = (y * cols) + x;
            c.flipped = false;
            c.matched = false;
            c.flipProgress = 0.0f; // Initialize to Face Down
            
            bool isCenter = (diff == DIFF_HARD && x == 2 && y == 2);
            
            if (isCenter) {
                c.active = false;
                c.id = -1;
                c.color = DARKGRAY;
            } else {
                c.active = true;
                c.id = ids[idCounter++];
                c.color = CARD_COLORS[c.id % 12]; 
            }

            cards.push_back(c);
        }
    }

    #if defined(PLATFORM_WEB)
        RefreshLeaderboard();
    #endif
    
}

void UpdateDrawFrame() {
    Vector2 mousePos = GetMousePosition();
    bool mouseClicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool enterPressed = IsKeyPressed(KEY_ENTER);
    float dt = GetFrameTime();

    // --- ANIMATION UPDATE ---
    // This runs globally so cards animate regardless of game state
    for (auto& card : cards) {
        if (!card.active) continue;

        float target = card.flipped ? 1.0f : 0.0f;
        
        if (card.flipProgress < target) {
            card.flipProgress += dt * FLIP_SPEED;
            if (card.flipProgress > target) card.flipProgress = target;
        } 
        else if (card.flipProgress > target) {
            card.flipProgress -= dt * FLIP_SPEED;
            if (card.flipProgress < target) card.flipProgress = target;
        }
    }

    // --- GAME UPDATE ---
    switch (currentState) {
        case STATE_MENU:
            if (mouseClicked) {
                Rectangle btnMedium = { SCREEN_WIDTH/2 - 100, 250, 200, 50 };
                Rectangle btnHard = { SCREEN_WIDTH/2 - 100, 320, 200, 50 };

                if (CheckCollisionPointRec(mousePos, btnMedium)) {
                    StartGame(DIFF_MEDIUM);
                } else if (CheckCollisionPointRec(mousePos, btnHard)) {
                    StartGame(DIFF_HARD);
                }
            }
            break;

        case STATE_PLAYING: {
            int cols = (currentDifficulty == DIFF_MEDIUM) ? 4 : 5;
            int rows = (currentDifficulty == DIFF_MEDIUM) ? 4 : 5;
            
            // --- KEYBOARD NAVIGATION ---
            int dx = 0;
            int dy = 0;
            if (IsKeyPressed(KEY_RIGHT)) dx = 1;
            if (IsKeyPressed(KEY_LEFT)) dx = -1;
            if (IsKeyPressed(KEY_DOWN)) dy = 1;
            if (IsKeyPressed(KEY_UP)) dy = -1;
            
            if (dx != 0 || dy != 0) {
                int cx = keyboardCursor % cols;
                int cy = keyboardCursor / cols;
                
                int nx = cx + dx;
                int ny = cy + dy;
                
                if (nx >= 0 && nx < cols && ny >= 0 && ny < rows) {
                    int nextIndex = ny * cols + nx;
                    if (!cards[nextIndex].active) {
                        nx += dx;
                        ny += dy;
                        if (nx >= 0 && nx < cols && ny >= 0 && ny < rows) {
                            nextIndex = ny * cols + nx;
                            keyboardCursor = nextIndex;
                        }
                    } else {
                        keyboardCursor = nextIndex;
                    }
                }
            }

            Card* cardToSelect = nullptr;

            if (mouseClicked) {
                for (auto& card : cards) {
                    if (card.active && CheckCollisionPointRec(mousePos, card.rect)) {
                        cardToSelect = &card;
                        keyboardCursor = card.gridIndex; 
                        break;
                    }
                }
            }
            else if (enterPressed) {
                if (keyboardCursor >= 0 && keyboardCursor < (int)cards.size()) {
                    Card& c = cards[keyboardCursor];
                    if (c.active) {
                        cardToSelect = &c;
                    }
                }
            }

            if (cardToSelect) {
                // Only allow selection if not already matched and not currently flipping/flipped
                // We check flipProgress < 0.5 to ensure we don't click a card that is already half-open
                if (!cardToSelect->matched && !cardToSelect->flipped && cardToSelect->flipProgress < 0.5f) {
                    cardToSelect->flipped = true;
                    cardSeen[cardToSelect->gridIndex] = true;
                    
                    if (!firstSelection) {
                        firstSelection = cardToSelect;
                    } else {
                        secondSelection = cardToSelect;
                        moves++;
                        currentState = STATE_WAITING;
                        waitTimer = GetTime();
                    }
                }
            }
            break;
        }

        case STATE_WAITING:
            if (GetTime() - waitTimer > 0.8) { 
                if (firstSelection->id == secondSelection->id) {
                    firstSelection->matched = true;
                    secondSelection->matched = true;
                    matchesFound++;
                    
                    if (matchesFound >= totalPairs) {
                        currentState = STATE_GAMEOVER;
                        SaveScoreToBrowser(moves);
                    } else {
                        currentState = STATE_PLAYING;
                    }
                } else {
                    // Logic for errors
                    bool errorDetected = false;
                    for (const auto& c : cards) {
                        if (c.active && c.id == firstSelection->id && c.gridIndex != firstSelection->gridIndex) {
                            if (cardSeen[c.gridIndex] && c.gridIndex != secondSelection->gridIndex) errorDetected = true;
                        }
                    }
                    if (!errorDetected) {
                        for (const auto& c : cards) {
                            if (c.active && c.id == secondSelection->id && c.gridIndex != secondSelection->gridIndex) {
                                if (cardSeen[c.gridIndex] && c.gridIndex != firstSelection->gridIndex) errorDetected = true;
                            }
                        }
                    }

                    if (errorDetected) errors++;

                    firstSelection->flipped = false;
                    secondSelection->flipped = false;
                    currentState = STATE_PLAYING;
                }
                firstSelection = nullptr;
                secondSelection = nullptr;
            }
            break;

        case STATE_GAMEOVER:
            if (mouseClicked || enterPressed) {
                InitGame();
            }
            break;
    }

    // --- DRAW ---
    BeginDrawing();
    ClearBackground(RAYWHITE);

    for(int i=0; i<SCREEN_WIDTH; i+=40) DrawLine(i, 0, i, SCREEN_HEIGHT, Fade(LIGHTGRAY, 0.3f));
    for(int i=0; i<SCREEN_HEIGHT; i+=40) DrawLine(0, i, SCREEN_WIDTH, i, Fade(LIGHTGRAY, 0.3f));

    if (currentState == STATE_MENU) {
        DrawText("MEMORY GAME", SCREEN_WIDTH/2 - MeasureText("MEMORY GAME", 60)/2, 150, 60, DARKGRAY);
        
        Rectangle btnMedium = { (float)SCREEN_WIDTH/2 - 100, 250, 200, 50 };
        Rectangle btnHard = { (float)SCREEN_WIDTH/2 - 100, 320, 200, 50 };
        
        Color medColor = CheckCollisionPointRec(mousePos, btnMedium) ? SKYBLUE : LIGHTGRAY;
        Color hardColor = CheckCollisionPointRec(mousePos, btnHard) ? PINK : LIGHTGRAY;

        DrawRectangleRec(btnMedium, medColor);
        DrawRectangleLinesEx(btnMedium, 2, DARKGRAY);
        DrawText("Medium (4x4)", (int)btnMedium.x + 20, (int)btnMedium.y + 10, 24, DARKGRAY);

        DrawRectangleRec(btnHard, hardColor);
        DrawRectangleLinesEx(btnHard, 2, DARKGRAY);
        DrawText("Hard (5x5)", (int)btnHard.x + 35, (int)btnHard.y + 10, 24, DARKGRAY);
        
    } 
    else if (currentState == STATE_GAMEOVER) {
        DrawText("YOU WIN!", SCREEN_WIDTH/2 - MeasureText("YOU WIN!", 60)/2, 150, 60, GOLD);
        
        const char* movesText = TextFormat("Total Moves: %i", moves);
        DrawText(movesText, SCREEN_WIDTH/2 - MeasureText(movesText, 30)/2, 240, 30, DARKGRAY);
        
        Color errColor = (errors == 0) ? GREEN : RED;
        const char* errText = TextFormat("Errors Made: %i", errors);
        DrawText(errText, SCREEN_WIDTH/2 - MeasureText(errText, 30)/2, 280, 30, errColor);
        
        if (errors == 0) {
            DrawText("PERFECT MEMORY!", SCREEN_WIDTH/2 - MeasureText("PERFECT MEMORY!", 20)/2, 320, 20, ORANGE);
        }

        DrawText("Click or Press Enter to Return to Menu", SCREEN_WIDTH/2 - MeasureText("Click or Press Enter to Return to Menu", 20)/2, 400, 20, LIGHTGRAY);
    }
    else {
        for (const auto& card : cards) {
            DrawCard(card);
        }
        DrawText(TextFormat("Moves: %i", moves), 20, 20, 20, DARKGRAY);
        DrawText(TextFormat("Errors: %i", errors), 20, 45, 20, MAROON);
    }

    EndDrawing();
}

void DrawCard(const Card& card) {
    if (!card.active) return;

    // --- ANIMATION CALCULATIONS ---
    // Visual flip logic:
    // 0.0 (Back) -> 0.5 (Edge) -> 1.0 (Front)
    // We determine scale based on distance from 0.5
    
    float animVal = card.flipProgress;
    bool showFront = (animVal >= 0.5f);
    
    // Calculate width scale: 
    // At animVal 0.0 -> scale 1.0
    // At animVal 0.5 -> scale 0.0
    // At animVal 1.0 -> scale 1.0
    float scaleX = fabsf(1.0f - (2.0f * animVal));

    // Create the drawing rectangle centered on the original position
    Rectangle r = card.rect;
    float originalWidth = r.width;
    r.width = originalWidth * scaleX;
    r.x = card.rect.x + (originalWidth - r.width) / 2.0f;

    bool isSelected = (currentState == STATE_PLAYING && card.gridIndex == keyboardCursor);

    // --- RENDER ---
    if (showFront) {
        // FACE UP
        if (card.matched) {
            DrawRectangleRec(r, Fade(card.color, 0.3f));
            DrawRectangleLinesEx(r, 2, Fade(card.color, 0.5f));
        } else {
            DrawRectangleRec(r, card.color);
            DrawRectangleLinesEx(r, 3, WHITE);
            // Simple symbol to give it texture
            DrawCircle(r.x + r.width/2, r.y + r.height/2, 10 * scaleX, WHITE);
        }
    } else {
        // FACE DOWN (BACK)
        DrawRectangleRec(r, DARKGRAY);
        DrawRectangleLinesEx(r, 3, GRAY);
        
        // Only draw pattern if width is sufficient to avoid ugly aliasing
        if (scaleX > 0.2f) {
            DrawLine(r.x, r.y, r.x + r.width, r.y + r.height, GRAY);
            DrawLine(r.x + r.width, r.y, r.x, r.y + r.height, GRAY);
        }
    }
    
    // Draw Selection Highlight
    // We draw this outside the scaling logic slightly so it tracks the flip
    if (isSelected) {
        DrawRectangleLinesEx(r, 5, SKYBLUE); 
    }
}