// ==WindhawkMod==
// @id              hello-pin-default-debug
// @name            Hello PIN default diagnostics
// @description     Passively traces LogonUI credential selection
// @version         0.3
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

This experimental, read-only mod observes `CredProvDataModel.dll` and installs
six pass-through tracing hooks for provider identity and credential-selection
transitions. It changes no arguments, return values, or authentication state.

Add `LogonUI.exe` to Windhawk's process inclusion list before enabling the mod.
After collecting the log, disable the mod. The trace contains provider GUIDs,
selection flags, and object addresses, but no credential contents.
*/
// ==/WindhawkModReadme==

#include <windows.h>

#include <cwchar>

namespace {

constexpr DWORD kPollIntervalMs = 250;
constexpr DWORD kPollAttempts = 120000 / kPollIntervalMs;

HANDLE g_stopEvent;
HANDLE g_workerThread;

using GetDefaultSelectedProviderId_t = HRESULT(__cdecl*)(void*, GUID*);
using GetCredentialGuid_t = HRESULT(__cdecl*)(void*, GUID*);
using SetDefaultSelection_t = HRESULT(__cdecl*)(void*, UINT, bool, int, bool);
using SelectAsync_t = HRESULT(__cdecl*)(void*, int);
using SetSelectedBucket_t = HRESULT(__cdecl*)(void*, void*, bool*);

GetDefaultSelectedProviderId_t GetDefaultSelectedProviderId_Original;
GetCredentialGuid_t GetProviderId_Original;
GetCredentialGuid_t GetClsid_Original;
SetDefaultSelection_t SetDefaultSelection_Original;
SelectAsync_t SelectAsync_Original;
SetSelectedBucket_t SetSelectedBucket_Original;

void LogGuid(PCWSTR label, const GUID& guid) {
    Wh_Log(L"[trace] %s={%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
           label, static_cast<unsigned>(guid.Data1),
           static_cast<unsigned>(guid.Data2),
           static_cast<unsigned>(guid.Data3),
           static_cast<unsigned>(guid.Data4[0]),
           static_cast<unsigned>(guid.Data4[1]),
           static_cast<unsigned>(guid.Data4[2]),
           static_cast<unsigned>(guid.Data4[3]),
           static_cast<unsigned>(guid.Data4[4]),
           static_cast<unsigned>(guid.Data4[5]),
           static_cast<unsigned>(guid.Data4[6]),
           static_cast<unsigned>(guid.Data4[7]));
}

HRESULT __cdecl GetDefaultSelectedProviderId_Hook(void* self,
                                                  GUID* providerId) {
    HRESULT result = GetDefaultSelectedProviderId_Original(self, providerId);
    Wh_Log(L"[trace] GetDefaultSelectedProviderId this=%p result=0x%08X",
           self, static_cast<unsigned>(result));
    if (SUCCEEDED(result) && providerId) {
        LogGuid(L"defaultProvider", *providerId);
    }
    return result;
}

HRESULT __cdecl GetProviderId_Hook(void* self, GUID* providerId) {
    HRESULT result = GetProviderId_Original(self, providerId);
    Wh_Log(L"[trace] get_ProviderId this=%p result=0x%08X", self,
           static_cast<unsigned>(result));
    if (SUCCEEDED(result) && providerId) {
        LogGuid(L"providerId", *providerId);
    }
    return result;
}

HRESULT __cdecl GetClsid_Hook(void* self, GUID* clsid) {
    HRESULT result = GetClsid_Original(self, clsid);
    Wh_Log(L"[trace] GetClsid this=%p result=0x%08X", self,
           static_cast<unsigned>(result));
    if (SUCCEEDED(result) && clsid) {
        LogGuid(L"providerClsid", *clsid);
    }
    return result;
}

HRESULT __cdecl SetDefaultSelection_Hook(void* self,
                                         UINT credentialCount,
                                         bool userSelected,
                                         int credentialsChangedState,
                                         bool unknownFlag) {
    Wh_Log(L"[trace] SetDefaultSelection this=%p count=%u userSelected=%u "
           L"changedState=%d flag=%u",
           self, credentialCount, userSelected, credentialsChangedState,
           unknownFlag);
    HRESULT result = SetDefaultSelection_Original(
        self, credentialCount, userSelected, credentialsChangedState,
        unknownFlag);
    Wh_Log(L"[trace] SetDefaultSelection result=0x%08X",
           static_cast<unsigned>(result));
    return result;
}

HRESULT __cdecl SelectAsync_Hook(void* self, int flags) {
    Wh_Log(L"[trace] SelectAsync this=%p flags=0x%08X", self,
           static_cast<unsigned>(flags));
    HRESULT result = SelectAsync_Original(self, flags);
    Wh_Log(L"[trace] SelectAsync result=0x%08X",
           static_cast<unsigned>(result));
    return result;
}

HRESULT __cdecl SetSelectedBucket_Hook(void* self,
                                       void* bucket,
                                       bool* selectionChanged) {
    Wh_Log(L"[trace] SetSelectedBucket this=%p bucket=%p", self, bucket);
    HRESULT result =
        SetSelectedBucket_Original(self, bucket, selectionChanged);
    Wh_Log(L"[trace] SetSelectedBucket result=0x%08X changed=%d",
           static_cast<unsigned>(result),
           selectionChanged ? static_cast<int>(*selectionChanged) : -1);
    return result;
}

struct SymbolTarget {
    PCWSTR decoratedName;
    void* address;
};

bool ResolveSymbols(HMODULE module, SymbolTarget* targets, size_t count) {
    WH_FIND_SYMBOL findData{};
    HANDLE search = Wh_FindFirstSymbol(module, nullptr, &findData);
    if (!search) {
        Wh_Log(L"CredProvDataModel.dll symbol enumeration could not start");
        return false;
    }

    size_t resolved = 0;
    do {
        if (!findData.symbolDecorated) {
            continue;
        }
        for (size_t i = 0; i < count; ++i) {
            if (!targets[i].address &&
                std::wcscmp(findData.symbolDecorated,
                            targets[i].decoratedName) == 0) {
                targets[i].address = findData.address;
                ++resolved;
                Wh_Log(L"Resolved trace symbol %s at %p",
                       targets[i].decoratedName, findData.address);
                break;
            }
        }
    } while (Wh_FindNextSymbol(search, &findData));

    Wh_FindCloseSymbol(search);
    if (resolved != count) {
        Wh_Log(L"Resolved %zu of %zu trace symbols; no hooks installed",
               resolved, count);
        return false;
    }

    return true;
}

bool InstallTraceHooks(HMODULE module) {
    SymbolTarget targets[] = {
        {L"?v_GetDefaultSelectedProviderId@CUserData@@MEAAJPEAU_GUID@@@Z",
         nullptr},
        {L"?_SetDefaultSelection@CCredProvDataModel@@AEAAJI_NW4CREDENTIALSCHANGED_STATE@@0@Z",
         nullptr},
        {L"?SelectAsync@CCredentialData@@UEAAJW4TILE_SELECTION_FLAGS@@@Z",
         nullptr},
        {L"?_SetSelectedBucket@CCredProvDataModel@@AEAAJPEAUICredentialBucket@CredProvData@Logon@UI@Internal@Windows@@PEA_N@Z",
         nullptr},
        {L"?get_ProviderId@CCredentialData@@UEAAJPEAU_GUID@@@Z", nullptr},
        {L"?GetClsid@CCredentialData@@UEAAJPEAU_GUID@@@Z", nullptr},
    };

    if (!ResolveSymbols(module, targets, ARRAYSIZE(targets))) {
        return false;
    }

    bool registered =
        Wh_SetFunctionHook(targets[0].address,
                           reinterpret_cast<void*>(
                               GetDefaultSelectedProviderId_Hook),
                           reinterpret_cast<void**>(
                               &GetDefaultSelectedProviderId_Original)) &&
        Wh_SetFunctionHook(
            targets[1].address,
            reinterpret_cast<void*>(SetDefaultSelection_Hook),
            reinterpret_cast<void**>(&SetDefaultSelection_Original)) &&
        Wh_SetFunctionHook(targets[2].address,
                           reinterpret_cast<void*>(SelectAsync_Hook),
                           reinterpret_cast<void**>(&SelectAsync_Original)) &&
        Wh_SetFunctionHook(targets[3].address,
                           reinterpret_cast<void*>(SetSelectedBucket_Hook),
                           reinterpret_cast<void**>(&SetSelectedBucket_Original)) &&
        Wh_SetFunctionHook(targets[4].address,
                           reinterpret_cast<void*>(GetProviderId_Hook),
                           reinterpret_cast<void**>(&GetProviderId_Original)) &&
        Wh_SetFunctionHook(targets[5].address,
                           reinterpret_cast<void*>(GetClsid_Hook),
                           reinterpret_cast<void**>(&GetClsid_Original));
    if (!registered) {
        Wh_Log(L"Trace hook registration failed; hooks not applied");
        return false;
    }

    if (!Wh_ApplyHookOperations()) {
        Wh_Log(L"Trace hook application failed");
        return false;
    }

    Wh_Log(L"Six passive selection trace hooks installed");
    return true;
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
            InstallTraceHooks(dataModel);
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
    Wh_Log(L"Init (passive trace only; no authentication changes)");

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
        CloseHandle(g_stopEvent);
        g_stopEvent = nullptr;
    }
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
