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
    STATE_HELP,
    STATE_PLAYING,
    STATE_WAITING, 
    STATE_GAMEOVER
};

enum Difficulty {
    DIFF_MEDIUM, // 4x4
    DIFF_HARD    // 5x5
};

// Helper struct to map Raylib Keys to readable chars
struct KeyDefinition {
    KeyboardKey key;
    char label[2]; // Null terminated string for drawing
};

struct Card {
    Rectangle rect;
    Color color;
    int id;       
    int gridIndex; 
    bool flipped; // Logical state (True = user wants to see it)
    bool matched;
    bool active;  
    
    // New Input Logic
    KeyboardKey assignedKey; 
    char keyLabel[2];

    // Animation State
    float flipProgress; // 0.0f (Back) to 1.0f (Front)
};

// --- Globals ---
std::vector<Card> cards;
GameState currentState = STATE_MENU;
Difficulty currentDifficulty = DIFF_MEDIUM;

Card* firstSelection = nullptr;
Card* secondSelection = nullptr;

// Stats
double waitTimer = 0.0;
int matchesFound = 0;
int moves = 0;
int errors = 0;
int totalPairs = 0;
int finalScore = 0; 

// Memory Tracking for Errors
std::vector<bool> cardSeen; 

// --- Function Prototypes ---
void InitGame();
void StartGame(Difficulty diff);
void UpdateDrawFrame(void);
void DrawCard(const Card& card);
std::vector<KeyDefinition> GetKeyPool(); 

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

// --- Helper: Generate Pool of Keys (0-9, A-Z) ---
std::vector<KeyDefinition> GetKeyPool() {
    std::vector<KeyDefinition> pool;
    
    // Add Numbers 0-9
    for (int i = 0; i <= 9; i++) {
        KeyDefinition k;
        k.key = (KeyboardKey)(KEY_ZERO + i);
        k.label[0] = '0' + i;
        k.label[1] = '\0';
        pool.push_back(k);
    }
    
    // Add Letters A-Z
    for (int i = 0; i < 26; i++) {
        KeyDefinition k;
        k.key = (KeyboardKey)(KEY_A + i);
        k.label[0] = 'A' + i;
        k.label[1] = '\0';
        pool.push_back(k);
    }
    return pool;
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
    finalScore = 0;
    firstSelection = nullptr;
    secondSelection = nullptr;
    currentDifficulty = diff;
    
    int rows = (diff == DIFF_MEDIUM) ? 4 : 5;
    int cols = (diff == DIFF_MEDIUM) ? 4 : 5;
    
    int totalSlots = rows * cols;
    totalPairs = (diff == DIFF_MEDIUM) ? 8 : 12;

    cardSeen.assign(totalSlots, false);

    // 1. Setup IDs for Pairs
    std::vector<int> ids;
    for (int i = 0; i < totalPairs; i++) {
        ids.push_back(i);
        ids.push_back(i);
    }

    // 2. Setup Input Keys (Randomized)
    std::vector<KeyDefinition> keyPool = GetKeyPool();

    // Random generators
    std::random_device rd;
    std::mt19937 g(rd());
    
    std::shuffle(ids.begin(), ids.end(), g);
    std::shuffle(keyPool.begin(), keyPool.end(), g); // Shuffle keys so 'A' isn't always top-left

    int gridWidth = (cols * CARD_SIZE) + ((cols - 1) * CARD_SPACING);
    int gridHeight = (rows * CARD_SIZE) + ((rows - 1) * CARD_SPACING);
    int offsetX = (SCREEN_WIDTH - gridWidth) / 2;
    int offsetY = (SCREEN_HEIGHT - gridHeight) / 2;

    int idCounter = 0;
    int keyCounter = 0;

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
            c.flipProgress = 0.0f; 
            
            bool isCenter = (diff == DIFF_HARD && x == 2 && y == 2);
            
            if (isCenter) {
                c.active = false;
                c.id = -1;
                c.color = DARKGRAY;
                // Center card gets no key
                c.assignedKey = KEY_NULL;
                c.keyLabel[0] = '\0';
            } else {
                c.active = true;
                c.id = ids[idCounter++];
                c.color = CARD_COLORS[c.id % 12]; 
                
                // Assign Random Key
                if (keyCounter < keyPool.size()) {
                    c.assignedKey = keyPool[keyCounter].key;
                    c.keyLabel[0] = keyPool[keyCounter].label[0];
                    c.keyLabel[1] = '\0';
                    keyCounter++;
                }
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
                Rectangle btnHelp = { SCREEN_WIDTH/2 - 100, 390, 200, 50 };

                if (CheckCollisionPointRec(mousePos, btnMedium)) {
                    StartGame(DIFF_MEDIUM);
                } else if (CheckCollisionPointRec(mousePos, btnHard)) {
                    StartGame(DIFF_HARD);
                } else if (CheckCollisionPointRec(mousePos, btnHelp)) {
                    currentState = STATE_HELP;
                }
            }
            break;

        case STATE_HELP:
            if (mouseClicked || enterPressed || IsKeyPressed(KEY_ESCAPE)) {
                currentState = STATE_MENU;
            }
            break;

        case STATE_PLAYING: {
            Rectangle btnMenu = { (float)SCREEN_WIDTH - 120, 20, 100, 30 };
            if (mouseClicked && CheckCollisionPointRec(mousePos, btnMenu)) {
                currentState = STATE_MENU;
                break; 
            }

            Card* cardToSelect = nullptr;

            // 1. Mouse Selection (Legacy support, but primarily for UI fallback)
            if (mouseClicked) {
                for (auto& card : cards) {
                    if (card.active && CheckCollisionPointRec(mousePos, card.rect)) {
                        cardToSelect = &card;
                        break;
                    }
                }
            }

            // 2. Direct Key Selection
            // Iterate over all active cards to see if their specific key was pressed
            if (cardToSelect == nullptr) {
                for (auto& card : cards) {
                    if (card.active && !card.matched && !card.flipped && IsKeyPressed(card.assignedKey)) {
                        cardToSelect = &card;
                        break; // Only handle one card press per frame
                    }
                }
            }

            if (cardToSelect) {
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

                        // --- SCORING LOGIC ---
                        float multiplier = (currentDifficulty == DIFF_MEDIUM) ? 1.5f : 1.0f;
                        finalScore = (int)((float)(moves + errors) * multiplier);
                        
                        SaveScoreToBrowser(finalScore);
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
        DrawText("MEMORY GAME", SCREEN_WIDTH/2 - MeasureText("MEMORY GAME", 60)/2, 130, 60, DARKGRAY);
        
        Rectangle btnMedium = { (float)SCREEN_WIDTH/2 - 100, 250, 200, 50 };
        Rectangle btnHard = { (float)SCREEN_WIDTH/2 - 100, 320, 200, 50 };
        Rectangle btnHelp = { (float)SCREEN_WIDTH/2 - 100, 390, 200, 50 };
        
        Color medColor = CheckCollisionPointRec(mousePos, btnMedium) ? SKYBLUE : LIGHTGRAY;
        Color hardColor = CheckCollisionPointRec(mousePos, btnHard) ? PINK : LIGHTGRAY;
        Color helpColor = CheckCollisionPointRec(mousePos, btnHelp) ? GOLD : LIGHTGRAY;

        DrawRectangleRec(btnMedium, medColor);
        DrawRectangleLinesEx(btnMedium, 2, DARKGRAY);
        DrawText("Medium (4x4)", (int)btnMedium.x + 20, (int)btnMedium.y + 10, 24, DARKGRAY);

        DrawRectangleRec(btnHard, hardColor);
        DrawRectangleLinesEx(btnHard, 2, DARKGRAY);
        DrawText("Hard (5x5)", (int)btnHard.x + 35, (int)btnHard.y + 10, 24, DARKGRAY);

        DrawRectangleRec(btnHelp, helpColor);
        DrawRectangleLinesEx(btnHelp, 2, DARKGRAY);
        DrawText("HOW TO PLAY", (int)btnHelp.x + 20, (int)btnHelp.y + 10, 24, DARKGRAY);
    } 
    else if (currentState == STATE_HELP) {
        // --- HELP SCREEN ---
        DrawText("HOW TO PLAY", SCREEN_WIDTH/2 - MeasureText("HOW TO PLAY", 40)/2, 60, 40, SKYBLUE);
        
        int y = 140;
        int x = 100;
        int fontSize = 20;
        int spacing = 35;

        DrawText("- Type the key shown on the card to flip it.", x, y, fontSize, DARKGRAY); y += spacing;
        DrawText("- Try to match pairs with the fewest moves.", x, y, fontSize, DARKGRAY); y += spacing;
        
        y += 10;
        DrawText("SCORING (Lower is Better!):", x, y, 22, GOLD); y += spacing;
        DrawText("Score = (Moves + Errors) x Difficulty Multiplier", x + 20, y, fontSize, DARKGRAY); y += spacing;
        DrawText("- Moves: Every pair of cards you flip.", x + 20, y, fontSize, DARKGRAY); y += spacing;
        DrawText("- Errors: Flipping a card you have seen before", x + 20, y, fontSize, RED); y += 22;
        DrawText("  but failing to match it.", x + 40, y, fontSize, RED); y += spacing + 10;
        
        DrawText("Hard Mode has no multiplier (1.0x).", x, y, fontSize, DARKGRAY); y += spacing;
        DrawText("Medium Mode has a penalty multiplier (1.5x).", x, y, fontSize, DARKGRAY); y += spacing;

        DrawText("Click or Press Enter to return", SCREEN_WIDTH/2 - MeasureText("Click or Press Enter to return", 20)/2, 530, 20, LIGHTGRAY);
    }
    else if (currentState == STATE_GAMEOVER) {
        DrawText("YOU WIN!", SCREEN_WIDTH/2 - MeasureText("YOU WIN!", 60)/2, 130, 60, GOLD);
        
        const char* movesText = TextFormat("Moves: %i   Errors: %i", moves, errors);
        DrawText(movesText, SCREEN_WIDTH/2 - MeasureText(movesText, 24)/2, 220, 24, DARKGRAY);

        const char* scoreText = TextFormat("FINAL SCORE: %i", finalScore);
        DrawText(scoreText, SCREEN_WIDTH/2 - MeasureText(scoreText, 40)/2, 270, 40, SKYBLUE);

        if (currentDifficulty == DIFF_MEDIUM) {
             DrawText("(Moves + Errors) x 1.5", SCREEN_WIDTH/2 - MeasureText("(Moves + Errors) x 1.5", 20)/2, 320, 20, LIGHTGRAY);
        } else {
             DrawText("(Moves + Errors) x 1.0", SCREEN_WIDTH/2 - MeasureText("(Moves + Errors) x 1.0", 20)/2, 320, 20, LIGHTGRAY);
        }
        
        if (finalScore < 20) {
            DrawText("INCREDIBLE MEMORY!", SCREEN_WIDTH/2 - MeasureText("INCREDIBLE MEMORY!", 20)/2, 360, 20, ORANGE);
        }

        DrawText("Click or Press Enter to Return to Menu", SCREEN_WIDTH/2 - MeasureText("Click or Press Enter to Return to Menu", 20)/2, 450, 20, LIGHTGRAY);
    }
    else {
        // Playing
        for (const auto& card : cards) {
            DrawCard(card);
        }
        DrawText(TextFormat("Moves: %i", moves), 20, 20, 20, DARKGRAY);
        DrawText(TextFormat("Errors: %i", errors), 20, 45, 20, MAROON);

        Rectangle btnMenu = { (float)SCREEN_WIDTH - 120, 20, 70, 30 };
        bool isHovering = CheckCollisionPointRec(mousePos, btnMenu);
        Color btnColor = isHovering ? MAROON : DARKGRAY;
        
        DrawRectangleRec(btnMenu, btnColor);
        DrawRectangleLinesEx(btnMenu, 2, WHITE);
        DrawText("MENU", (int)btnMenu.x + 8, (int)btnMenu.y + 8, 12, RAYWHITE);
    }

    EndDrawing();
}

void DrawCard(const Card& card) {
    if (!card.active) return;

    // --- ANIMATION CALCULATIONS ---
    float animVal = card.flipProgress;
    bool showFront = (animVal >= 0.5f);
    
    float scaleX = fabsf(1.0f - (2.0f * animVal));

    Rectangle r = card.rect;
    float originalWidth = r.width;
    r.width = originalWidth * scaleX;
    r.x = card.rect.x + (originalWidth - r.width) / 2.0f;

    // --- RENDER ---
    if (showFront) {
        // FACE UP
        if (card.matched) {
            DrawRectangleRec(r, Fade(card.color, 0.3f));
            DrawRectangleLinesEx(r, 2, Fade(card.color, 0.5f));
        } else {
            DrawRectangleRec(r, card.color);
            DrawRectangleLinesEx(r, 3, WHITE);
            // Simple symbol
            DrawCircle(r.x + r.width/2, r.y + r.height/2, 10 * scaleX, WHITE);
        }
    } else {
        // FACE DOWN (BACK)
        DrawRectangleRec(r, DARKGRAY);
        DrawRectangleLinesEx(r, 3, GRAY);
        
        // DRAW THE ASSIGNED KEY 
        // Only draw if width is sufficient
        if (scaleX > 0.4f && !card.matched) {
            int fontSize = 40;
            // Center the text
            int textWidth = MeasureText(card.keyLabel, fontSize);
            // Dynamic scaling correction for the text so it squishes with the card
            // (Optional polish: can just not draw if scale is low)
            
            DrawText(card.keyLabel, 
                (int)(r.x + (r.width - textWidth * scaleX)/2), 
                (int)(r.y + (r.height - fontSize)/2), 
                fontSize, 
                LIGHTGRAY
            );
        }
    }
}