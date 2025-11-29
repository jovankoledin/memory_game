#include "js_interop.h"
#include <cstdio> // For printf in the desktop stubs

// --- EM_JS Definitions (Used only when PLATFORM_WEB is defined) ---
#if defined(PLATFORM_WEB)
#include <emscripten.h>

// CORRECT SYNTAX: Parameters are in one set of parentheses: (int score, int sortOrder)
EM_JS(void, SaveScoreToBrowser, (int score, int sortOrder), {
    if (typeof window.updateLeaderboard === 'function') {
        window.updateLeaderboard(score, sortOrder);
    } else {
        console.warn("updateLeaderboard() missing in window");
    }
});

// CORRECT SYNTAX: No parameters are in one set of parentheses: ()
EM_JS(void, RefreshLeaderboard, (), {
    if (typeof window.refreshLeaderboard === 'function') {
        window.refreshLeaderboard();
    } else {
        console.warn("refreshLeaderboard() missing in window");
    }
});

// --- Desktop stub definitions (Used when PLATFORM_WEB is NOT defined) ---
#else 
// NOTE: For a clean fix, you MUST ensure that SaveScoreToBrowser and RefreshLeaderboard
// in js_interop.h are wrapped in 'extern "C"' (conditionally, or always).
void SaveScoreToBrowser(int score, int sortOrder) {
    const char* mode = (sortOrder == 1) ? "High is Better" : "Low is Better";
    printf("[Desktop Stub] Score Saved: %d (%s)\n", score, mode);
}

void RefreshLeaderboard() {
    printf("[Desktop Stub] Refresh Leaderboard\n");
}
#endif