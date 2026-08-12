#pragma once

#include <windows.h>

// Minimal declarations for local syntax-only compilation. Windhawk provides
// these definitions when compiling a mod in the application.
typedef struct tagWH_FIND_SYMBOL {
    void* address;
    PCWSTR symbol;
    PCWSTR symbolDecorated;
} WH_FIND_SYMBOL;

extern "C" HANDLE Wh_FindFirstSymbol(HMODULE, const void*, WH_FIND_SYMBOL*);
extern "C" BOOL Wh_FindNextSymbol(HANDLE, WH_FIND_SYMBOL*);
extern "C" void Wh_FindCloseSymbol(HANDLE);
extern "C" BOOL Wh_SetFunctionHook(void*, void*, void**);
extern "C" BOOL Wh_ApplyHookOperations();
extern "C" void Wh_Log(PCWSTR, ...);
