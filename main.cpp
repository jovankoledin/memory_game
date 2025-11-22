#include "raylib.h"
#include <vector>
#include <algorithm>
#include <random>
#include <string>

// Required for Web assembly compilation
#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

// --- Constants & Config ---
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int COLS = 4;
const int ROWS = 4;
const int CARD_SIZE = 100;
const int CARD_SPACING = 20;
const int GRID_OFFSET_X = (SCREEN_WIDTH - ((COLS * CARD_SIZE) + ((COLS - 1) * CARD_SPACING))) / 2;
const int GRID_OFFSET_Y = (SCREEN_HEIGHT - ((ROWS * CARD_SIZE) + ((ROWS - 1) * CARD_SPACING))) / 2;

// --- Colors for pairs ---
const Color CARD_COLORS[] = {
    RED, ORANGE, YELLOW, GREEN, SKYBLUE, BLUE, PURPLE, PINK
};

// --- Game State ---
enum GameState {
    STATE_MENU,
    STATE_PLAYING,
    STATE_WAITING, // Waiting for 2nd card animation/check
    STATE_GAMEOVER
};

struct Card {
    Rectangle rect;
    Color color;
    int id;       // Matches another card with same ID
    bool flipped;
    bool matched;
};

// --- Globals ---
std::vector<Card> cards;
GameState currentState = STATE_MENU;
Card* firstSelection = nullptr;
Card* secondSelection = nullptr;
double waitTimer = 0.0;
int matchesFound = 0;
int moves = 0;

// --- Function Prototypes ---
void InitGame();
void UpdateDrawFrame(void);
void DrawCard(const Card& card);
void ResetGame();

// --- Main Entry Point ---
int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Raylib Web Memory Game");

    InitGame();

#if defined(PLATFORM_WEB)
    // Emscripten requires this specific main loop callback
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

void ResetGame() {
    cards.clear();
    matchesFound = 0;
    moves = 0;
    firstSelection = nullptr;
    secondSelection = nullptr;
    currentState = STATE_PLAYING;

    // Create pairs
    std::vector<int> ids;
    for (int i = 0; i < (COLS * ROWS) / 2; i++) {
        ids.push_back(i);
        ids.push_back(i);
    }

    // Shuffle
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(ids.begin(), ids.end(), g);

    // Build Grid
    int idIndex = 0;
    for (int y = 0; y < ROWS; y++) {
        for (int x = 0; x < COLS; x++) {
            Card c;
            c.rect = {
                (float)(GRID_OFFSET_X + x * (CARD_SIZE + CARD_SPACING)),
                (float)(GRID_OFFSET_Y + y * (CARD_SIZE + CARD_SPACING)),
                (float)CARD_SIZE,
                (float)CARD_SIZE
            };
            c.id = ids[idIndex];
            c.color = CARD_COLORS[c.id % 8]; // Map ID to color
            c.flipped = false;
            c.matched = false;
            cards.push_back(c);
            idIndex++;
        }
    }
}

void InitGame() {
    ResetGame();
    currentState = STATE_MENU; // Start at menu
}

void UpdateDrawFrame() {
    // --- UPDATE ---
    Vector2 mousePos = GetMousePosition();
    bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    switch (currentState) {
        case STATE_MENU:
            if (clicked) {
                ResetGame();
                currentState = STATE_PLAYING;
            }
            break;

        case STATE_PLAYING:
            if (clicked) {
                for (auto& card : cards) {
                    if (!card.matched && !card.flipped && CheckCollisionPointRec(mousePos, card.rect)) {
                        card.flipped = true;
                        
                        if (!firstSelection) {
                            firstSelection = &card;
                        } else {
                            secondSelection = &card;
                            moves++;
                            currentState = STATE_WAITING;
                            waitTimer = GetTime();
                        }
                        break; // Only click one at a time
                    }
                }
            }
            break;

        case STATE_WAITING:
            if (GetTime() - waitTimer > 1.0) { // Wait 1 second
                if (firstSelection->id == secondSelection->id) {
                    firstSelection->matched = true;
                    secondSelection->matched = true;
                    matchesFound++;
                    if (matchesFound >= (COLS * ROWS) / 2) {
                        currentState = STATE_GAMEOVER;
                    } else {
                        currentState = STATE_PLAYING;
                    }
                } else {
                    firstSelection->flipped = false;
                    secondSelection->flipped = false;
                    currentState = STATE_PLAYING;
                }
                firstSelection = nullptr;
                secondSelection = nullptr;
            }
            break;

        case STATE_GAMEOVER:
            if (clicked) {
                ResetGame();
                currentState = STATE_PLAYING;
            }
            break;
    }

    // --- DRAW ---
    BeginDrawing();
    ClearBackground(RAYWHITE);

    // Draw Background Pattern
    for(int i=0; i<SCREEN_WIDTH; i+=40) {
        DrawLine(i, 0, i, SCREEN_HEIGHT, Fade(LIGHTGRAY, 0.3f));
    }
    for(int i=0; i<SCREEN_HEIGHT; i+=40) {
        DrawLine(0, i, SCREEN_WIDTH, i, Fade(LIGHTGRAY, 0.3f));
    }

    if (currentState == STATE_MENU) {
        DrawText("MEMORY GAME", SCREEN_WIDTH/2 - MeasureText("MEMORY GAME", 60)/2, 200, 60, DARKGRAY);
        DrawText("Click to Start", SCREEN_WIDTH/2 - MeasureText("Click to Start", 30)/2, 300, 30, DARKGRAY);
    } 
    else if (currentState == STATE_GAMEOVER) {
        DrawText("YOU WIN!", SCREEN_WIDTH/2 - MeasureText("YOU WIN!", 60)/2, 200, 60, GOLD);
        const char* movesText = TextFormat("Moves: %i", moves);
        DrawText(movesText, SCREEN_WIDTH/2 - MeasureText(movesText, 30)/2, 280, 30, DARKGRAY);
        DrawText("Click to Play Again", SCREEN_WIDTH/2 - MeasureText("Click to Play Again", 20)/2, 350, 20, LIGHTGRAY);
    }
    else {
        // Draw Grid
        for (const auto& card : cards) {
            DrawCard(card);
        }
        DrawText(TextFormat("Moves: %i", moves), 20, 20, 20, DARKGRAY);
    }

    EndDrawing();
}

void DrawCard(const Card& card) {
    if (card.matched) {
        // Ghost/faded out style for matched cards
        DrawRectangleRec(card.rect, Fade(card.color, 0.3f));
        DrawRectangleLinesEx(card.rect, 2, Fade(card.color, 0.5f));
    } else if (card.flipped) {
        // Face up
        DrawRectangleRec(card.rect, card.color);
        DrawRectangleLinesEx(card.rect, 3, WHITE);
        // Simple icon style detail
        DrawCircle(card.rect.x + card.rect.width/2, card.rect.y + card.rect.height/2, 10, WHITE);
    } else {
        // Face down (Card back)
        DrawRectangleRec(card.rect, DARKGRAY);
        DrawRectangleLinesEx(card.rect, 3, GRAY);
        // Pattern on back
        DrawLine(card.rect.x, card.rect.y, card.rect.x + card.rect.width, card.rect.y + card.rect.height, GRAY);
        DrawLine(card.rect.x + card.rect.width, card.rect.y, card.rect.x, card.rect.y + card.rect.height, GRAY);
    }
}