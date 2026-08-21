// ==WindhawkMod==
// @id              photoshop-wheel-modifiers
// @name            Photoshop wheel modifier remap
// @description     Makes Ctrl+wheel zoom, Shift+wheel scroll horizontally, and Alt+wheel scroll quickly
// @version         0.1
// @author          PRO-2684
// @github          https://github.com/PRO-2684
// @homepage        https://pro-2684.github.io/
// @include         Photoshop.exe
// @license         MIT
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Photoshop wheel modifier remap

Remaps Photoshop's canvas mouse wheel controls to:

- **Ctrl + wheel**: zoom
- **Shift + wheel**: horizontal scroll
- **Alt + wheel**: fast vertical scroll
- **Alt + Shift + wheel**: fast horizontal scroll

Keep **Preferences > Tools > Zoom With Scroll Wheel** disabled.

See the [full README](https://github.com/PRO-2684/WindHawk-mods/tree/main/mods/photoshop-wheel-modifiers) for details, compatibility notes, and troubleshooting.
*/
// ==/WindhawkModReadme==

#include <windows.h>


namespace {

using ModifierMask = unsigned;

constexpr ModifierMask kModifierNone = 0;
constexpr ModifierMask kModifierCtrl = 1u << 0;
constexpr ModifierMask kModifierShift = 1u << 1;
constexpr ModifierMask kModifierAlt = 1u << 2;

thread_local ModifierMask g_modifierOverride = kModifierNone;

using GetMessageA_t = decltype(&GetMessageA);
using GetMessageW_t = decltype(&GetMessageW);
using PeekMessageA_t = decltype(&PeekMessageA);
using PeekMessageW_t = decltype(&PeekMessageW);
using GetKeyState_t = decltype(&GetKeyState);
using GetAsyncKeyState_t = decltype(&GetAsyncKeyState);
using GetKeyboardState_t = decltype(&GetKeyboardState);

GetMessageA_t GetMessageA_Original;
GetMessageW_t GetMessageW_Original;
PeekMessageA_t PeekMessageA_Original;
PeekMessageW_t PeekMessageW_Original;
GetKeyState_t GetKeyState_Original;
GetAsyncKeyState_t GetAsyncKeyState_Original;
GetKeyboardState_t GetKeyboardState_Original;

const wchar_t* ModifierName(ModifierMask modifier) {
    switch (modifier) {
        case kModifierNone:
            return L"None";
        case kModifierCtrl:
            return L"Ctrl";
        case kModifierShift:
            return L"Shift";
        case kModifierAlt:
            return L"Alt";
        case kModifierCtrl | kModifierShift:
            return L"Ctrl+Shift";
        case kModifierCtrl | kModifierAlt:
            return L"Ctrl+Alt";
        case kModifierShift | kModifierAlt:
            return L"Shift+Alt";
        case kModifierCtrl | kModifierShift | kModifierAlt:
            return L"Ctrl+Shift+Alt";
        default:
            return L"?";
    }
}

const wchar_t* VirtualKeyName(int virtualKey) {
    switch (virtualKey) {
        case VK_CONTROL:
            return L"VK_CONTROL";
        case VK_LCONTROL:
            return L"VK_LCONTROL";
        case VK_RCONTROL:
            return L"VK_RCONTROL";
        case VK_SHIFT:
            return L"VK_SHIFT";
        case VK_LSHIFT:
            return L"VK_LSHIFT";
        case VK_RSHIFT:
            return L"VK_RSHIFT";
        case VK_MENU:
            return L"VK_MENU";
        case VK_LMENU:
            return L"VK_LMENU";
        case VK_RMENU:
            return L"VK_RMENU";
        default:
            return L"other";
    }
}

bool IsModifierKey(int virtualKey) {
    switch (virtualKey) {
        case VK_CONTROL:
        case VK_LCONTROL:
        case VK_RCONTROL:
        case VK_SHIFT:
        case VK_LSHIFT:
        case VK_RSHIFT:
        case VK_MENU:
        case VK_LMENU:
        case VK_RMENU:
            return true;
        default:
            return false;
    }
}

bool IsKeyForModifier(int virtualKey, ModifierMask modifier) {
    const bool isCtrl =
        virtualKey == VK_CONTROL ||
        virtualKey == VK_LCONTROL ||
        virtualKey == VK_RCONTROL;
    const bool isShift =
        virtualKey == VK_SHIFT ||
        virtualKey == VK_LSHIFT ||
        virtualKey == VK_RSHIFT;
    const bool isAlt =
        virtualKey == VK_MENU ||
        virtualKey == VK_LMENU ||
        virtualKey == VK_RMENU;

    if (isCtrl) {
        return (modifier & kModifierCtrl) != 0;
    }
    if (isShift) {
        return (modifier & kModifierShift) != 0;
    }
    if (isAlt) {
        return (modifier & kModifierAlt) != 0;
    }

    return false;
}

bool KeyDownFromState(SHORT state) {
    return (state & static_cast<SHORT>(0x8000)) != 0;
}

bool KeyDownFromKeyboardState(const BYTE* keyState, int virtualKey) {
    return (keyState[virtualKey] & 0x80) != 0;
}

void GetWindowClass(HWND hwnd, wchar_t* buffer, int bufferChars) {
    if (!buffer || bufferChars <= 0) {
        return;
    }

    buffer[0] = L'\0';
    if (!hwnd) {
        return;
    }

    if (!GetClassNameW(hwnd, buffer, bufferChars)) {
        buffer[0] = L'\0';
    }
}

void LogMessage(const wchar_t* stage, const MSG* message) {
    if (!message || message->message != WM_MOUSEWHEEL) {
        return;
    }

    wchar_t className[128];
    GetWindowClass(message->hwnd, className, ARRAYSIZE(className));

    const UINT mouseKeyState = LOWORD(message->wParam);
    const short wheelDelta = static_cast<short>(HIWORD(message->wParam));

    const bool ctrlFromMessage = (mouseKeyState & MK_CONTROL) != 0;
    const bool shiftFromMessage = (mouseKeyState & MK_SHIFT) != 0;
    const bool ctrlPhysical =
        GetAsyncKeyState_Original &&
        KeyDownFromState(GetAsyncKeyState_Original(VK_CONTROL));
    const bool shiftPhysical =
        GetAsyncKeyState_Original &&
        KeyDownFromState(GetAsyncKeyState_Original(VK_SHIFT));
    const bool altPhysical =
        GetAsyncKeyState_Original &&
        KeyDownFromState(GetAsyncKeyState_Original(VK_MENU));

    Wh_Log(
        L"%s WM_MOUSEWHEEL hwnd=0x%p class=\"%s\" wParam=0x%llX "
        L"lParam=0x%llX delta=%d msgKeys=[ctrl=%d shift=%d] "
        L"physical=[ctrl=%d shift=%d alt=%d]",
        stage,
        message->hwnd,
        className,
        static_cast<unsigned long long>(message->wParam),
        static_cast<unsigned long long>(message->lParam),
        static_cast<int>(wheelDelta),
        ctrlFromMessage,
        shiftFromMessage,
        ctrlPhysical,
        shiftPhysical,
        altPhysical);
}

SHORT OverrideModifierState(int virtualKey, SHORT originalState) {
    if (g_modifierOverride == kModifierNone || !IsModifierKey(virtualKey)) {
        return originalState;
    }

    // Preserve the low toggle bit and replace only the "currently down" bit.
    SHORT result = originalState & 0x0001;
    if (IsKeyForModifier(virtualKey, g_modifierOverride)) {
        result |= static_cast<SHORT>(0x8000);
    }
    return result;
}

SHORT WINAPI GetKeyState_Hook(int virtualKey) {
    SHORT originalState = GetKeyState_Original(virtualKey);
    SHORT state = OverrideModifierState(virtualKey, originalState);

    if (g_modifierOverride != kModifierNone && IsModifierKey(virtualKey)) {
        Wh_Log(
            L"GetKeyState(%s): originalDown=%d returnedDown=%d override=%s",
            VirtualKeyName(virtualKey),
            KeyDownFromState(originalState),
            KeyDownFromState(state),
            ModifierName(g_modifierOverride));
    }

    return state;
}

SHORT WINAPI GetAsyncKeyState_Hook(int virtualKey) {
    SHORT originalState = GetAsyncKeyState_Original(virtualKey);
    SHORT state = OverrideModifierState(virtualKey, originalState);

    if (g_modifierOverride != kModifierNone && IsModifierKey(virtualKey)) {
        Wh_Log(
            L"GetAsyncKeyState(%s): originalDown=%d returnedDown=%d override=%s",
            VirtualKeyName(virtualKey),
            KeyDownFromState(originalState),
            KeyDownFromState(state),
            ModifierName(g_modifierOverride));
    }

    return state;
}

void SetKeyboardModifierState(PBYTE keyState, int virtualKey, bool down) {
    if (down) {
        keyState[virtualKey] |= 0x80;
    } else {
        keyState[virtualKey] &= 0x7F;
    }
}

BOOL WINAPI GetKeyboardState_Hook(PBYTE keyState) {
    BOOL result = GetKeyboardState_Original(keyState);
    if (!result || !keyState) {
        return result;
    }

    bool originalCtrl = KeyDownFromKeyboardState(keyState, VK_CONTROL);
    bool originalShift = KeyDownFromKeyboardState(keyState, VK_SHIFT);
    bool originalAlt = KeyDownFromKeyboardState(keyState, VK_MENU);

    if (g_modifierOverride != kModifierNone) {
        constexpr int modifierKeys[] = {
            VK_CONTROL, VK_LCONTROL, VK_RCONTROL,
            VK_SHIFT, VK_LSHIFT, VK_RSHIFT,
            VK_MENU, VK_LMENU, VK_RMENU,
        };

        for (int virtualKey : modifierKeys) {
            SetKeyboardModifierState(
                keyState,
                virtualKey,
                IsKeyForModifier(virtualKey, g_modifierOverride));
        }
    }

    if (g_modifierOverride != kModifierNone) {
        Wh_Log(
            L"GetKeyboardState: original=[ctrl=%d shift=%d alt=%d] "
            L"returned=[ctrl=%d shift=%d alt=%d] override=%s",
            originalCtrl,
            originalShift,
            originalAlt,
            KeyDownFromKeyboardState(keyState, VK_CONTROL),
            KeyDownFromKeyboardState(keyState, VK_SHIFT),
            KeyDownFromKeyboardState(keyState, VK_MENU),
            ModifierName(g_modifierOverride));
    }

    return result;
}

ModifierMask GetPhysicalWheelModifier(WPARAM wParam) {
    const UINT mouseKeyState = LOWORD(wParam);

    ModifierMask modifier = kModifierNone;

    if ((mouseKeyState & MK_CONTROL) != 0) {
        modifier |= kModifierCtrl;
    }
    if ((mouseKeyState & MK_SHIFT) != 0) {
        modifier |= kModifierShift;
    }

    // WM_MOUSEWHEEL doesn't carry an Alt bit, so query the original key state.
    if (GetKeyState_Original &&
        KeyDownFromState(GetKeyState_Original(VK_MENU))) {
        modifier |= kModifierAlt;
    }

    return modifier;
}

ModifierMask RemapModifier(ModifierMask modifier) {
    switch (modifier) {
        case kModifierCtrl:
            return kModifierAlt;
        case kModifierShift:
            return kModifierCtrl;
        case kModifierAlt:
            return kModifierShift;
        case kModifierShift | kModifierAlt:
            return kModifierCtrl | kModifierShift;
        default:
            return kModifierNone;
    }
}

WPARAM RewriteWheelKeyState(WPARAM wParam, ModifierMask modifier) {
    UINT mouseKeyState = LOWORD(wParam);
    mouseKeyState &= ~(MK_CONTROL | MK_SHIFT);

    if (modifier & kModifierCtrl) {
        mouseKeyState |= MK_CONTROL;
    }
    if (modifier & kModifierShift) {
        mouseKeyState |= MK_SHIFT;
    }

    return MAKEWPARAM(mouseKeyState, HIWORD(wParam));
}

void ClearModifierOverride(const wchar_t* source) {
    if (g_modifierOverride != kModifierNone) {
        Wh_Log(
            L"%s: clearing wheel modifier override=%s",
            source,
            ModifierName(g_modifierOverride));
        g_modifierOverride = kModifierNone;
    }
}

void RemapRetrievedWheelMessage(MSG* message, const wchar_t* source) {
    if (!message || message->message != WM_MOUSEWHEEL) {
        return;
    }

    // Initial evidence shows Photoshop's canvas wheel messages target PSViewC.
    // Keep the prototype narrow so modifier state can't leak into unrelated UI.
    wchar_t className[128];
    GetWindowClass(message->hwnd, className, ARRAYSIZE(className));
    if (wcscmp(className, L"PSViewC") != 0) {
        Wh_Log(
            L"%s: WM_MOUSEWHEEL target class=\"%s\"; leaving unchanged",
            source,
            className);
        return;
    }

    ModifierMask physicalModifier = GetPhysicalWheelModifier(message->wParam);
    ModifierMask remappedModifier = RemapModifier(physicalModifier);

    Wh_Log(
        L"%s: WM_MOUSEWHEEL remap decision physical=%s remapped=%s",
        source,
        ModifierName(physicalModifier),
        ModifierName(remappedModifier));

    if (remappedModifier == kModifierNone) {
        return;
    }

    WPARAM oldWParam = message->wParam;
    message->wParam =
        RewriteWheelKeyState(message->wParam, remappedModifier);
    g_modifierOverride = remappedModifier;

    Wh_Log(
        L"%s: WM_MOUSEWHEEL rewritten oldWParam=0x%llX newWParam=0x%llX "
        L"override=%s",
        source,
        static_cast<unsigned long long>(oldWParam),
        static_cast<unsigned long long>(message->wParam),
        ModifierName(g_modifierOverride));
}

BOOL WINAPI GetMessageA_Hook(LPMSG message,
                             HWND hwnd,
                             UINT messageFilterMin,
                             UINT messageFilterMax) {
    // Re-entering GetMessage means Photoshop has finished processing the
    // previously retrieved message on this thread.
    ClearModifierOverride(L"GetMessageA");

    BOOL result =
        GetMessageA_Original(message, hwnd, messageFilterMin, messageFilterMax);
    if (result > 0) {
        LogMessage(L"GetMessageA(before)", message);
        RemapRetrievedWheelMessage(message, L"GetMessageA");
        LogMessage(L"GetMessageA(after)", message);
    }
    return result;
}

BOOL WINAPI GetMessageW_Hook(LPMSG message,
                             HWND hwnd,
                             UINT messageFilterMin,
                             UINT messageFilterMax) {
    // Re-entering GetMessage means Photoshop has finished processing the
    // previously retrieved message on this thread.
    ClearModifierOverride(L"GetMessageW");

    BOOL result =
        GetMessageW_Original(message, hwnd, messageFilterMin, messageFilterMax);
    if (result > 0) {
        LogMessage(L"GetMessageW(before)", message);
        RemapRetrievedWheelMessage(message, L"GetMessageW");
        LogMessage(L"GetMessageW(after)", message);
    }
    return result;
}

BOOL WINAPI PeekMessageA_Hook(LPMSG message,
                              HWND hwnd,
                              UINT messageFilterMin,
                              UINT messageFilterMax,
                              UINT removeMsg) {
    BOOL result = PeekMessageA_Original(
        message, hwnd, messageFilterMin, messageFilterMax, removeMsg);
    if (!result) {
        return result;
    }

    if (removeMsg & PM_REMOVE) {
        ClearModifierOverride(L"PeekMessageA(remove)");
        LogMessage(L"PeekMessageA(before)", message);
        RemapRetrievedWheelMessage(message, L"PeekMessageA");
        LogMessage(L"PeekMessageA(after)", message);
    } else {
        LogMessage(L"PeekMessageA(peek)", message);
    }

    return result;
}

BOOL WINAPI PeekMessageW_Hook(LPMSG message,
                              HWND hwnd,
                              UINT messageFilterMin,
                              UINT messageFilterMax,
                              UINT removeMsg) {
    BOOL result = PeekMessageW_Original(
        message, hwnd, messageFilterMin, messageFilterMax, removeMsg);
    if (!result) {
        return result;
    }

    if (removeMsg & PM_REMOVE) {
        ClearModifierOverride(L"PeekMessageW(remove)");
        LogMessage(L"PeekMessageW(before)", message);
        RemapRetrievedWheelMessage(message, L"PeekMessageW");
        LogMessage(L"PeekMessageW(after)", message);
    } else {
        LogMessage(L"PeekMessageW(peek)", message);
    }

    return result;
}

bool InstallHooks() {
#define INSTALL_HOOK(function)                                                   \
    do {                                                                         \
        if (!Wh_SetFunctionHook(                                                 \
                reinterpret_cast<void*>(function),                               \
                reinterpret_cast<void*>(function##_Hook),                        \
                reinterpret_cast<void**>(&function##_Original))) {               \
            Wh_Log(L"Failed to hook " L ## #function);                               \
            return false;                                                        \
        }                                                                        \
    } while (false)

    INSTALL_HOOK(GetMessageA);
    INSTALL_HOOK(GetMessageW);
    INSTALL_HOOK(PeekMessageA);
    INSTALL_HOOK(PeekMessageW);

    INSTALL_HOOK(GetKeyState);
    INSTALL_HOOK(GetAsyncKeyState);
    INSTALL_HOOK(GetKeyboardState);


#undef INSTALL_HOOK
    return true;
}

}  // namespace

BOOL Wh_ModInit() {
    Wh_Log(L"Init Photoshop wheel modifier remap");

    if (!InstallHooks()) {
        Wh_Log(L"Hook installation failed; behavior unchanged");
        return FALSE;
    }

    Wh_Log(L"Hooks installed");
    return TRUE;
}

void Wh_ModUninit() {
    Wh_Log(L"Uninit");
}
