// ==WindhawkMod==
// @id              hello-pin-default-debug
// @name            Hello PIN default diagnostics
// @description     Logs LogonUI credential-model modules and selection-related public symbols
// @version         0.1
// @author          PRO-2684
// @github          https://github.com/PRO-2684
// @homepage        https://pro-2684.github.io/
// @include         LogonUI.exe
// @compilerOptions -lcomdlg32
// @license         MIT
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Hello PIN default diagnostics

This experimental, read-only mod observes which credential-model DLLs load
naturally in `LogonUI.exe`. When `CredProvDataModel.dll` appears, it queries
Microsoft public symbols through Windhawk and logs names likely related to
credential selection. It installs no hooks and changes no authentication state.

Add `LogonUI.exe` to Windhawk's process inclusion list before enabling the mod.
After collecting the log, disable the mod. Symbol enumeration can take time on
the first run while Windhawk populates its symbol cache.
*/
// ==/WindhawkModReadme==

#include <windows.h>

#include <cwctype>

namespace {

constexpr DWORD kPollIntervalMs = 250;
constexpr DWORD kPollAttempts = 120000 / kPollIntervalMs;

HANDLE g_stopEvent;
HANDLE g_workerThread;

bool ContainsCaseInsensitive(PCWSTR text, PCWSTR needle) {
    if (!text || !needle || !*needle) {
        return false;
    }

    for (; *text; ++text) {
        PCWSTR textCursor = text;
        PCWSTR needleCursor = needle;
        while (*textCursor && *needleCursor &&
               std::towlower(*textCursor) == std::towlower(*needleCursor)) {
            ++textCursor;
            ++needleCursor;
        }
        if (!*needleCursor) {
            return true;
        }
    }

    return false;
}

bool IsInterestingSymbol(PCWSTR symbol, PCWSTR decoratedSymbol) {
    constexpr PCWSTR kKeywords[] = {
        L"Credential", L"Provider", L"Select", L"Selected", L"Default",
        L"Fallback",   L"Selector", L"Method", L"Bucket",   L"User",
        L"PIN",        L"NGC",      L"Bio",
    };

    for (PCWSTR keyword : kKeywords) {
        if (ContainsCaseInsensitive(symbol, keyword) ||
            ContainsCaseInsensitive(decoratedSymbol, keyword)) {
            return true;
        }
    }

    return false;
}

void EnumerateInterestingSymbols(HMODULE module) {
    WH_FIND_SYMBOL findData{};
    HANDLE search = Wh_FindFirstSymbol(module, nullptr, &findData);
    if (!search) {
        Wh_Log(L"CredProvDataModel.dll symbol enumeration could not start");
        return;
    }

    DWORD total = 0;
    DWORD matched = 0;
    do {
        ++total;
        if (!IsInterestingSymbol(findData.symbol, findData.symbolDecorated)) {
            continue;
        }

        ++matched;
        if (findData.symbol && findData.symbolDecorated) {
            Wh_Log(L"Symbol %p: %s | decorated: %s", findData.address,
                   findData.symbol, findData.symbolDecorated);
        } else {
            Wh_Log(L"Symbol %p: %s", findData.address,
                   findData.symbol ? findData.symbol
                                   : findData.symbolDecorated);
        }
    } while (Wh_FindNextSymbol(search, &findData));

    Wh_FindCloseSymbol(search);
    Wh_Log(L"CredProvDataModel.dll symbols: %lu matched out of %lu", matched,
           total);
}

DWORD WINAPI DiagnosticWorker(LPVOID) {
    struct ModuleState {
        PCWSTR name;
        bool logged;
    } modules[] = {
        {L"authui.dll", false},
        {L"CredProvCommonCore.dll", false},
        {L"CredProvDataModel.dll", false},
    };

    Wh_Log(L"Watching for credential-model modules for up to 120 seconds");

    for (DWORD attempt = 0; attempt < kPollAttempts; ++attempt) {
        HMODULE dataModel = nullptr;
        for (size_t i = 0; i < ARRAYSIZE(modules); ++i) {
            auto& module = modules[i];
            HMODULE handle = GetModuleHandleW(module.name);
            if (handle && !module.logged) {
                module.logged = true;
                Wh_Log(L"Naturally loaded: %s at %p", module.name, handle);
            }
            if (handle && i == 2) {
                dataModel = handle;
            }
        }

        if (dataModel) {
            EnumerateInterestingSymbols(dataModel);
            return 0;
        }

        if (WaitForSingleObject(g_stopEvent, kPollIntervalMs) != WAIT_TIMEOUT) {
            Wh_Log(L"Diagnostic worker stopped before symbol enumeration");
            return 0;
        }
    }

    Wh_Log(L"CredProvDataModel.dll was not observed within 120 seconds");
    return 0;
}

}  // namespace

BOOL Wh_ModInit() {
    Wh_Log(L"Init (diagnostic only; no hooks or authentication changes)");

    g_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g_stopEvent) {
        Wh_Log(L"CreateEventW failed, error %lu; diagnostics disabled",
               GetLastError());
        return TRUE;
    }

    g_workerThread = CreateThread(nullptr, 0, DiagnosticWorker, nullptr, 0,
                                  nullptr);
    if (!g_workerThread) {
        Wh_Log(L"CreateThread failed, error %lu; diagnostics disabled",
               GetLastError());
        CloseHandle(g_stopEvent);
        g_stopEvent = nullptr;
    }

    return TRUE;
}

void Wh_ModUninit() {
    Wh_Log(L"Uninit");

    if (g_stopEvent) {
        SetEvent(g_stopEvent);
    }
    if (g_workerThread) {
        WaitForSingleObject(g_workerThread, INFINITE);
        CloseHandle(g_workerThread);
        g_workerThread = nullptr;
    }
    if (g_stopEvent) {
        CloseHandle(g_stopEvent);
        g_stopEvent = nullptr;
    }
}
