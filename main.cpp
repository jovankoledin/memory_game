#include "raylib.h"
#include <vector>
#include <algorithm>
#include <random>
#include <string>
#include <map>

// Required for Web assembly compilation
#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

#if defined(PLATFORM_WEB)
    EM_JS(void, SaveScoreToBrowser, (int score), {
        if (typeof window.updateLeaderboard === 'function') {
            window.updateLeaderboard(score);
        } else {
            console.log("Leaderboard function not found in HTML");
        }
    });
    EM_JS(void, RefreshLeaderboard, (), {
        if (typeof window.refreshLeaderboard === 'function') {
            window.refreshLeaderboard();
        } else {
            console.log("Leaderboard refresh function not found in HTML");
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
const int CARD_SIZE = 90; // Slightly smaller to fit 5x5 comfortably
const int CARD_SPACING = 15;

// --- Colors for pairs (Expanded for Hard Mode) ---
const Color CARD_COLORS[] = {
    RED, ORANGE, YELLOW, GREEN, SKYBLUE, BLUE, PURPLE, PINK,
    LIME, GOLD, MAROON, DARKBLUE // Added more colors for 12 pairs
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
    int id;       // Matches another card with same ID
    int gridIndex; // To track unique position
    bool flipped;
    bool matched;
    bool active;  // False for the empty center slot in 5x5
};

// --- Globals ---
std::vector<Card> cards;
GameState currentState = STATE_MENU;
Difficulty currentDifficulty = DIFF_MEDIUM;

Card* firstSelection = nullptr;
Card* secondSelection = nullptr;

// Input State
int keyboardCursor = 0; // Index of the currently highlighted card

// Stats
double waitTimer = 0.0;
int matchesFound = 0;
int moves = 0;
int errors = 0;
int totalPairs = 0;

// Memory Tracking for Errors
std::vector<bool> cardSeen; // Tracks if a specific grid index has been revealed

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
    keyboardCursor = 0; // Reset cursor to top-left

    int rows = (diff == DIFF_MEDIUM) ? 4 : 5;
    int cols = (diff == DIFF_MEDIUM) ? 4 : 5;
    
    // In 5x5, we have 25 slots. We use 24 cards (12 pairs) and leave center empty.
    int totalSlots = rows * cols;
    totalPairs = (diff == DIFF_MEDIUM) ? 8 : 12;

    // Reset memory tracker
    cardSeen.assign(totalSlots, false);

    // Create IDs
    std::vector<int> ids;
    for (int i = 0; i < totalPairs; i++) {
        ids.push_back(i);
        ids.push_back(i);
    }

    // Shuffle IDs
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(ids.begin(), ids.end(), g);

    // Calculate centering offsets
    int gridWidth = (cols * CARD_SIZE) + ((cols - 1) * CARD_SPACING);
    int gridHeight = (rows * CARD_SIZE) + ((rows - 1) * CARD_SPACING);
    int offsetX = (SCREEN_WIDTH - gridWidth) / 2;
    int offsetY = (SCREEN_HEIGHT - gridHeight) / 2;

    // Build Grid
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
            
            // Handle 5x5 center empty slot
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

    // --- UPDATE ---
    switch (currentState) {
        case STATE_MENU:
            if (mouseClicked) {
                // Simple button collision logic for Menu
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
                
                // Bounds Check
                if (nx >= 0 && nx < cols && ny >= 0 && ny < rows) {
                    int nextIndex = ny * cols + nx;
                    
                    // Skip inactive card (Middle of 5x5)
                    if (!cards[nextIndex].active) {
                        nx += dx;
                        ny += dy;
                        // Re-check bounds after jump
                        if (nx >= 0 && nx < cols && ny >= 0 && ny < rows) {
                            nextIndex = ny * cols + nx;
                            keyboardCursor = nextIndex;
                        }
                    } else {
                        keyboardCursor = nextIndex;
                    }
                }
            }

            // --- SELECTION LOGIC ---
            Card* cardToSelect = nullptr;

            // 1. Check Mouse Click
            if (mouseClicked) {
                for (auto& card : cards) {
                    if (card.active && CheckCollisionPointRec(mousePos, card.rect)) {
                        cardToSelect = &card;
                        // Sync keyboard cursor to mouse position for continuity
                        keyboardCursor = card.gridIndex; 
                        break;
                    }
                }
            }
            // 2. Check Enter Key
            else if (enterPressed) {
                if (keyboardCursor >= 0 && keyboardCursor < (int)cards.size()) {
                    Card& c = cards[keyboardCursor];
                    if (c.active) {
                        cardToSelect = &c;
                    }
                }
            }

            // Apply Logic if we have a valid selection
            if (cardToSelect) {
                if (!cardToSelect->matched && !cardToSelect->flipped) {
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
            if (GetTime() - waitTimer > 0.8) { // 0.8s delay
                if (firstSelection->id == secondSelection->id) {
                    // --- MATCH ---
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
                    // --- MISMATCH (Check for Errors) ---
                    bool errorDetected = false;

                    // Find partner of first selection
                    for (const auto& c : cards) {
                        if (c.active && c.id == firstSelection->id && c.gridIndex != firstSelection->gridIndex) {
                            if (cardSeen[c.gridIndex] && c.gridIndex != secondSelection->gridIndex) {
                                errorDetected = true;
                            }
                        }
                    }

                    // Find partner of second selection (if error not already found)
                    if (!errorDetected) {
                        for (const auto& c : cards) {
                            if (c.active && c.id == secondSelection->id && c.gridIndex != secondSelection->gridIndex) {
                                if (cardSeen[c.gridIndex] && c.gridIndex != firstSelection->gridIndex) {
                                    errorDetected = true;
                                }
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
                InitGame(); // Go back to menu
            }
            break;
    }

    // --- DRAW ---
    BeginDrawing();
    ClearBackground(RAYWHITE);

    // Draw Background Pattern
    for(int i=0; i<SCREEN_WIDTH; i+=40) DrawLine(i, 0, i, SCREEN_HEIGHT, Fade(LIGHTGRAY, 0.3f));
    for(int i=0; i<SCREEN_HEIGHT; i+=40) DrawLine(0, i, SCREEN_WIDTH, i, Fade(LIGHTGRAY, 0.3f));

    if (currentState == STATE_MENU) {
        DrawText("MEMORY GAME", SCREEN_WIDTH/2 - MeasureText("MEMORY GAME", 60)/2, 150, 60, DARKGRAY);
        
        // Draw Buttons
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
        
        DrawText("Use Mouse or Arrow Keys + Enter", SCREEN_WIDTH/2 - MeasureText("Use Mouse or Arrow Keys + Enter", 20)/2, 450, 20, GRAY);
    } 
    else if (currentState == STATE_GAMEOVER) {
        DrawText("YOU WIN!", SCREEN_WIDTH/2 - MeasureText("YOU WIN!", 60)/2, 150, 60, GOLD);
        
        const char* movesText = TextFormat("Total Moves: %i", moves);
        DrawText(movesText, SCREEN_WIDTH/2 - MeasureText(movesText, 30)/2, 240, 30, DARKGRAY);
        
        // NEW: Error Stat
        Color errColor = (errors == 0) ? GREEN : RED;
        const char* errText = TextFormat("Errors Made: %i", errors);
        DrawText(errText, SCREEN_WIDTH/2 - MeasureText(errText, 30)/2, 280, 30, errColor);
        
        if (errors == 0) {
            DrawText("PERFECT MEMORY!", SCREEN_WIDTH/2 - MeasureText("PERFECT MEMORY!", 20)/2, 320, 20, ORANGE);
        }

        DrawText("Click or Press Enter to Return to Menu", SCREEN_WIDTH/2 - MeasureText("Click or Press Enter to Return to Menu", 20)/2, 400, 20, LIGHTGRAY);
    }
    else {
        // Draw Grid
        for (const auto& card : cards) {
            DrawCard(card);
        }
        // HUD
        DrawText(TextFormat("Moves: %i", moves), 20, 20, 20, DARKGRAY);
        DrawText(TextFormat("Errors: %i", errors), 20, 45, 20, MAROON);
        
        // Instructions
        DrawText("Arrows to Move, Enter to Select", 20, SCREEN_HEIGHT - 30, 20, LIGHTGRAY);
    }

    EndDrawing();
}

void DrawCard(const Card& card) {
    // Do not draw the inactive center card in 5x5 mode
    if (!card.active) return;

    Rectangle r = card.rect;
    
    // Calculate highlight state
    // We only show selection highlight if we are in Playing state
    bool isSelected = (currentState == STATE_PLAYING && card.gridIndex == keyboardCursor);

    if (card.matched) {
        DrawRectangleRec(r, Fade(card.color, 0.3f));
        DrawRectangleLinesEx(r, 2, Fade(card.color, 0.5f));
    } else if (card.flipped) {
        DrawRectangleRec(r, card.color);
        DrawRectangleLinesEx(r, 3, WHITE);
        DrawCircle(r.x + r.width/2, r.y + r.height/2, 10, WHITE);
    } else {
        DrawRectangleRec(r, DARKGRAY);
        DrawRectangleLinesEx(r, 3, GRAY);
        // Hatch pattern
        DrawLine(r.x, r.y, r.x + r.width, r.y + r.height, GRAY);
        DrawLine(r.x + r.width, r.y, r.x, r.y + r.height, GRAY);
    }
    
    // Draw Keyboard Selection Highlight
    if (isSelected) {
        DrawRectangleLinesEx(r, 5, SKYBLUE); // Thick blue border for selection
    }
}