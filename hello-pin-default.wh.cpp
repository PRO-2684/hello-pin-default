// ==WindhawkMod==
// @id              hello-pin-default
// @name            Make Windows Hello PIN the default
// @description     Selects the NGC PIN credential instead of WinBio by default
// @version         0.1
// @author          PRO-2684
// @github          https://github.com/PRO-2684
// @homepage        https://pro-2684.github.io/
// @include         LogonUI.exe
// @license         MIT
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Make Windows Hello PIN the default

This experimental prototype changes the initial credential provider selected
for a known user tile from the WinBio fingerprint provider to the NGC Windows
Hello PIN provider. It calls Windows' original selection function first and
only replaces an exact WinBio result; all other results remain unchanged.

Fingerprint authentication is expected to remain active while PIN is selected,
but this must be verified across lock, sign-in, boot, failure, and multi-user
scenarios before the mod is considered production-safe.

Add `LogonUI.exe` to Windhawk's process inclusion list. Disable
`hello-pin-default-debug` before enabling this mod; do not run both together.
Disable this mod to roll back immediately.
*/
// ==/WindhawkModReadme==

#include <windows.h>

#include <cwchar>

namespace {

constexpr DWORD kPollIntervalMs = 250;
constexpr DWORD kPollAttempts = 120000 / kPollIntervalMs;

constexpr GUID kWinBioProvider = {
    0xBEC09223, 0xB018, 0x416D, {0xA0, 0xAC, 0x52, 0x39, 0x71, 0xB6, 0x39, 0xF5}};
constexpr GUID kNgcProvider = {
    0xD6886603, 0x9D2F, 0x4EB2, {0xB6, 0x67, 0x19, 0x71, 0x04, 0x1F, 0xA9, 0x6B}};

constexpr PCWSTR kDefaultProviderSymbol =
    L"?v_GetDefaultSelectedProviderId@CUserData@@MEAAJPEAU_GUID@@@Z";

HANDLE g_stopEvent;
HANDLE g_workerThread;

using GetDefaultSelectedProviderId_t = HRESULT(__cdecl*)(void*, GUID*);
GetDefaultSelectedProviderId_t GetDefaultSelectedProviderId_Original;

HRESULT __cdecl GetDefaultSelectedProviderId_Hook(void* self,
                                                  GUID* providerId) {
    HRESULT result = GetDefaultSelectedProviderId_Original(self, providerId);
    if (SUCCEEDED(result) && providerId &&
        IsEqualGUID(*providerId, kWinBioProvider)) {
        *providerId = kNgcProvider;
        Wh_Log(L"Changed default provider from WinBio to NGC PIN");
    }
    return result;
}

void* ResolveDefaultProviderSymbol(HMODULE module) {
    WH_FIND_SYMBOL findData{};
    HANDLE search = Wh_FindFirstSymbol(module, nullptr, &findData);
    if (!search) {
        Wh_Log(L"Default-provider symbol enumeration could not start");
        return nullptr;
    }

    void* address = nullptr;
    do {
        if (findData.symbolDecorated &&
            std::wcscmp(findData.symbolDecorated,
                        kDefaultProviderSymbol) == 0) {
            address = findData.address;
            break;
        }
    } while (Wh_FindNextSymbol(search, &findData));

    Wh_FindCloseSymbol(search);
    return address;
}

bool InstallHook(HMODULE module) {
    void* target = ResolveDefaultProviderSymbol(module);
    if (!target) {
        Wh_Log(L"Default-provider symbol not found; behavior unchanged");
        return false;
    }

    if (!Wh_SetFunctionHook(
            target,
            reinterpret_cast<void*>(GetDefaultSelectedProviderId_Hook),
            reinterpret_cast<void**>(&GetDefaultSelectedProviderId_Original))) {
        Wh_Log(L"Default-provider hook registration failed");
        return false;
    }

    if (!Wh_ApplyHookOperations()) {
        Wh_Log(L"Default-provider hook application failed");
        return false;
    }

    Wh_Log(L"Default-provider hook installed");
    return true;
}

DWORD WINAPI HookWorker(LPVOID) {
    for (DWORD attempt = 0; attempt < kPollAttempts; ++attempt) {
        HMODULE module = GetModuleHandleW(L"CredProvDataModel.dll");
        if (module) {
            InstallHook(module);
            return 0;
        }

        if (WaitForSingleObject(g_stopEvent, kPollIntervalMs) != WAIT_TIMEOUT) {
            return 0;
        }
    }

    Wh_Log(L"CredProvDataModel.dll was not observed; behavior unchanged");
    return 0;
}

}  // namespace

BOOL Wh_ModInit() {
    Wh_Log(L"Init experimental PIN-default prototype");

    g_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g_stopEvent) {
        Wh_Log(L"CreateEventW failed; behavior unchanged");
        return TRUE;
    }

    g_workerThread = CreateThread(nullptr, 0, HookWorker, nullptr, 0, nullptr);
    if (!g_workerThread) {
        Wh_Log(L"CreateThread failed; behavior unchanged");
        CloseHandle(g_stopEvent);
        g_stopEvent = nullptr;
    }

    return TRUE;
}

void Wh_ModBeforeUninit() {
    if (g_stopEvent) {
        SetEvent(g_stopEvent);
    }
    if (g_workerThread) {
        WaitForSingleObject(g_workerThread, INFINITE);
        CloseHandle(g_workerThread);
        g_workerThread = nullptr;
    }
}

void Wh_ModUninit() {
    Wh_Log(L"Uninit");
    if (g_stopEvent) {
        CloseHandle(g_stopEvent);
        g_stopEvent = nullptr;
    }
}
