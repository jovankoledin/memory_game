// js_interop.h
#ifndef JS_INTEROP_H
#define JS_INTEROP_H

#ifdef __cplusplus
// Only apply C linkage if compiling with C++ compiler
extern "C" {
#endif

void SaveScoreToBrowser(int score, int sortOrder);
void RefreshLeaderboard();

#ifdef __cplusplus
} // End extern "C"
#endif

#endif // JS_INTEROP_H