# Windhawk: Make PIN the selected Windows Hello sign-in method

## Objective

Implement the smallest safe Windhawk mod that makes the **PIN credential UI selected/focused by default for an already-known user tile in Windows LogonUI**, while leaving fingerprint authentication active in parallel.

The user originally wanted this UX:

- Touch fingerprint sensor → fingerprint authentication proceeds normally.
- Start typing → PIN input should immediately receive the keystrokes, without clicking **Sign-in options → PIN** first.

A key empirical observation simplifies the problem:

> **When the PIN sign-in option is selected, fingerprint authentication still works. When fingerprint is selected, keyboard typing does not automatically switch/focus PIN.**

Therefore the preferred design is now simply:

```text
selected/default credential UI = PIN

keyboard    -> PIN field
fingerprint -> Windows Hello biometric path still works
```

Do **not** build a keyboard-triggered provider switch unless forcing PIN selection proves impossible.

---

## Current conclusion

There is no existing Windhawk catalog mod that implements this behavior.

The likely implementation layer is **`CredProvDataModel.dll`**, not raw keyboard interception in `LogonUI.exe` and not a custom Credential Provider. The immediate research target is the logic that chooses the selected credential/provider for an identified user, especially the code associated with:

- `fallbackdefaultselector.cpp`
- `SelectCredentialProvider`
- credential selection / selected credential / default selector symbols
- `CredentialMethodKind`
- `CredentialBucket`
- selected-user transitions

Start with **diagnostic symbol enumeration/logging only**, then add a narrow selection hook after the relevant function has been identified.

---

## What was tried

### 1. Search existing Windhawk mods

The supplied `en.json` Windhawk catalog was searched for terms around:

- `LogonUI.exe`
- fingerprint / biometric
- Windows Hello
- credential provider
- PIN / password
- login / logon / sign-in

No mod implements PIN/fingerprint co-selection or automatic sign-in-option selection.

Only two useful references intentionally target `LogonUI.exe`:

1. `uxtheme-hook.wh.cpp` — minimal proof that a normal Windhawk mod can run in `LogonUI.exe` and hook ordinary APIs/symbols.
2. `win7-login-fade.wh.cpp` — larger reference showing `LogonUI.exe`/`winlogon.exe` injection, defensive initialization, `Wh_SetFunctionHook`, and `WindhawkUtils::HookSymbols`.

Use **`uxtheme-hook.wh.cpp` as the minimal structural reference** and **`win7-login-fade.wh.cpp` as the symbol-hooking/critical-process reference**.

### 2. Registry `DefaultCredentialProvider` policy

Registered providers on the test machine included:

```text
{60b78e88-ead8-445c-9cfd-0b87f74ea6cd} PasswordProvider
{8AF662BF-65A0-4D0A-A540-A338A999D36F} FaceCredentialProvider
{BEC09223-B018-416D-A0AC-523971B639F5} WinBio Credential Provider
{cb82ea12-9f71-446d-89e1-8d0924e1256e} PINLogonProvider
{D6886603-9D2F-4EB2-B667-1971041FA96B} NGC Credential Provider
{F8A1793B-7873-4046-B2A7-1F318747F427} FIDO Credential Provider
```

The following policy was tested:

```text
HKLM\SOFTWARE\Policies\Microsoft\Windows\System
  DefaultCredentialProvider = {cb82ea12-9f71-446d-89e1-8d0924e1256e}  (REG_SZ)
```

It **did not change the normal named-user tile**.

Reason: that policy is documented as selecting the default provider on the **“Other user” tile**. “Other user” is the generic login tile where a user identity is not already fixed; it is distinct from the normal tile already associated with a specific user/SID. Our problem concerns the latter.

Rollback command if the policy value is still present:

```powershell
Remove-ItemProperty `
  -Path 'HKLM:\SOFTWARE\Policies\Microsoft\Windows\System' `
  -Name DefaultCredentialProvider
```

No provider should be disabled or filtered out; fingerprint must remain available.

---

## Important provider-ID caveat

Do **not** assume `{cb82...}` is necessarily the exact runtime credential object that corresponds to the PIN UI we want.

The machine exposes both:

```text
PINLogonProvider     {cb82ea12-9f71-446d-89e1-8d0924e1256e}
NGC Credential Provider {D6886603-9D2F-4EB2-B667-1971041FA96B}
```

Windows Hello PIN is part of the NGC/Hello stack, and internal UI selection may be represented by provider GUID, credential method kind, credential bucket, or another model object. **Instrument/log the actual runtime selection before hard-coding a CLSID.**

---

## Binary reconnaissance

All binaries in this archive came from the user's machine.

### Versions and PDB identities

| Binary | File version | PDB | PDB GUID | Age |
|---|---|---|---|---:|
| `LogonUI.exe` | `10.0.26100.8972` | `logonui.pdb` | `44FE4D0E-A79B-3A53-E87F-C2A0CD0B0177` | 1 |
| `authui.dll` | `10.0.26100.8972` | `authui.pdb` | `0AA06B4A-BEB2-5A64-4DC7-C3A5CA445009` | 1 |
| `logoncli.dll` | `10.0.26100.8521` | `logoncli.pdb` | `514C731E-E004-4FBE-163A-5F92DB9D26D9` | 1 |
| `CredProvCommonCore.dll` | `10.0.26100.8972` | `CredProvCommonCore.pdb` | `4ABF6F30-8400-4C33-2D69-0CEC11FDD24D` | 1 |
| `CredProvDataModel.dll` | `10.0.26100.8972` | `credprovdatamodel.pdb` | `638B9226-FD97-B384-6C50-4925BDBCCF78` | 1 |

These values were extracted directly from each PE's RSDS debug record and version resource.

### Relevant static observations

#### `LogonUI.exe`

- Small host executable (~72 KiB in supplied build).
- Do not assume the credential-selection implementation lives directly in this EXE.

#### `authui.dll`

- Imports `CredProvCommonCore.dll` directly.
- Contains registry strings under `Software\Microsoft\Windows\CurrentVersion\Authentication\LogonUI`.
- Contains UI/model strings including `SelectedUserSID` and `ChoiceTile`.
- Useful secondary target, but current evidence points deeper into `CredProvDataModel.dll`.

#### `logoncli.dll`

- Appears to be Net Logon client/domain-networking machinery.
- Not currently considered relevant; deprioritize it.

#### `CredProvDataModel.dll`

This is currently the most promising binary. Embedded strings include:

```text
shellcommon\shell\auth\authux\credprovdatamodel\lib\fallbackdefaultselector.cpp
CCredProvDataModel_put_SelectedUser_Activity
SelectCredentialProvider
Windows.Internal.UI.Logon.CredProvData.CredentialBucket
Windows.Internal.UI.Logon.CredProvData.CredentialMethodKind
```

These strongly suggest the DLL owns the data model/default-selection logic needed for this project.

---

## PDB / symbol workflow

Do not search random PDB downloads. Microsoft Windows PDBs should be resolved from the Microsoft public symbol server using the exact RSDS identity.

With Debugging Tools for Windows installed, `symchk.exe` can populate a local cache, e.g.:

```powershell
symchk.exe C:\Windows\System32\authui.dll `
  /s srv*C:\symbols*https://msdl.microsoft.com/download/symbols

symchk.exe C:\Windows\System32\CredProvCommonCore.dll `
  /s srv*C:\symbols*https://msdl.microsoft.com/download/symbols

symchk.exe C:\Windows\System32\CredProvDataModel.dll `
  /s srv*C:\symbols*https://msdl.microsoft.com/download/symbols
```

However, a Windhawk diagnostic mod can query symbols directly. The supplied Windhawk wiki documents:

```cpp
Wh_FindFirstSymbol(...)
Wh_FindNextSymbol(...)
Wh_FindCloseSymbol(...)
```

Passing `NULL` as the symbol server uses the Microsoft public symbol server.

The existing `win7-login-fade` reference also demonstrates `WindhawkUtils::HookSymbols` for installing a hook by symbol name.

---

## Recommended next implementation phase

### Phase A — diagnostic mod only

Create a new Windhawk mod with approximately:

```text
@include LogonUI.exe
```

The user-provided Windhawk template is included as `new-mod-template.wh.cpp`.

Do not change authentication behavior yet.

#### A1. Determine which modules are naturally loaded

At runtime in `LogonUI.exe`, log whether these are present:

```text
authui.dll
CredProvCommonCore.dll
CredProvDataModel.dll
```

`Wh_ModInit` normally runs before the target process begins executing, so a module may not yet be loaded. **Do not force-load an authentication DLL merely for convenience** until its initialization semantics are understood.

For an initial diagnostic prototype, a small worker thread that periodically checks `GetModuleHandleW` can be acceptable. Keep it bounded and unload-safe. A later production version should use a cleaner module-load strategy if necessary.

#### A2. Enumerate PDB symbols in `CredProvDataModel.dll`

Once loaded, enumerate symbols and log names containing terms such as:

```text
Credential
Provider
Select
Selected
Default
Fallback
Selector
Method
Bucket
User
PIN
NGC
Bio
```

Pay particular attention to symbols whose source/class names correspond to `fallbackdefaultselector.cpp` or `SelectCredentialProvider`.

Save the Windhawk log. This is the most useful next artifact for the coding agent.

#### A3. Identify the state transition experimentally

With diagnostic logging enabled:

1. Reach the normal named-user logon tile.
2. Select fingerprint.
3. Select PIN.
4. Switch back and forth several times.
5. Lock/unlock and sign out/log in separately.

Correlate function calls/model changes with sign-in-option selection.

The target is the **selection state change**, not credential serialization or verification.

---

## Phase B — passive function hooks

After useful symbols are found, hook the smallest candidate function(s) in pass-through mode:

```cpp
Hook(args...) {
    Wh_Log(...);
    return Original(args...);
}
```

Log enough information to determine:

- current user / whether this is an identified user tile,
- currently selected credential/provider/method,
- candidate PIN method identity,
- candidate biometric method identity,
- when the fallback/default-selection algorithm runs,
- whether selection differs between sign-in, unlock, and post-failure states.

Avoid logging secret material, PIN contents, serialization buffers, or credential secrets.

---

## Phase C — minimal behavioral change

Once the selection API/model is known, modify **only the default/initial selected credential** so PIN wins for the identified user tile.

Desired invariant:

```text
PIN tile/input is foreground/focused
biometric provider remains enumerated and active
```

Avoid:

- disabling `WinBio Credential Provider`,
- filtering out the fingerprint credential,
- wrapping Microsoft Credential Providers,
- implementing a new Credential Provider,
- intercepting PIN text,
- changing authentication serialization,
- keyboard replay/synthetic input unless the simpler selection change fails.

If PIN-selected mode continues to accept fingerprint, no dynamic switching is needed.

---

## Test matrix

Minimum tests after a behavioral prototype exists:

| Scenario | Expected |
|---|---|
| Lock → unlock | PIN UI already selected; typing immediately enters PIN |
| Lock → touch fingerprint | Unlock succeeds without switching away from PIN |
| Sign out → sign in | Same behavior on fresh logon session |
| Cold boot → sign in | Same behavior if applicable |
| Bad fingerprint attempt | PIN UI remains directly usable |
| Wrong PIN | Normal Windows behavior remains intact |
| Sign-in options UI | Other methods remain selectable |
| Multiple user accounts | Do not accidentally force an inappropriate credential globally |

Also test whether Windows remembers the manually chosen credential across sessions; avoid fighting deliberate user selection more often than necessary. The ideal hook may affect only *initial/default selection*, not every later selection event.

---

## Safety / development constraints

`LogonUI.exe` is security-sensitive and Windhawk treats critical/system-process injection specially. The reference mods explicitly instruct users to add `LogonUI.exe` to Windhawk's process inclusion list.

Development rules:

1. Start read-only: symbol enumeration and logging.
2. Prefer public-PDB symbol hooks over hard-coded offsets.
3. Fail open: if a symbol/version is unknown, return `FALSE` from initialization or leave behavior untouched.
4. Do not touch credential secret buffers.
5. Keep original-call pass-through paths intact.
6. Test on lock/unlock before relying on the mod at boot.
7. Maintain an easy rollback path by disabling/removing the Windhawk mod.
8. Do not disable fingerprint provider as a workaround.

---

## Windhawk API notes from supplied wiki

Relevant facts from `Creating-a-new-mod.md`:

- A mod is one C++ file compiled into a DLL and loaded in the target process context.
- `@include LogonUI.exe` targets that process.
- `Wh_ModInit()` runs before target execution unless injecting into an already-running process.
- `Wh_SetFunctionHook` registers detours; Windhawk applies them after initialization.
- `Wh_FindFirstSymbol` / `Wh_FindNextSymbol` enumerate symbols for a loaded module and can use Microsoft's public symbol server.
- Current documented toolchain: Clang 20 / llvm-mingw, C++23 (Windhawk 1.7 documentation).

---

## Suggested project name / mod metadata

No final name was chosen. Keep the initial diagnostic mod clearly marked as experimental, e.g. an internal ID such as:

```text
hello-pin-default-debug
```

Do not publish until the behavior is version-tolerant and safe.

---

## Archive contents

```text
HANDOFF.md                              this document
MANIFEST.sha256                        SHA-256 hashes
new-mod-template.wh.cpp                template supplied by user
sources/Creating-a-new-mod.md           Windhawk wiki page supplied by user
sources/en.json                         downloaded Windhawk mod catalog
sources/windhawk/uxtheme-hook.wh.cpp    LogonUI injection reference
sources/windhawk/win7-login-fade.wh.cpp symbol-hooking/critical-process reference
sources/windows/LogonUI.exe             machine binary
sources/windows/authui.dll              machine binary
sources/windows/logoncli.dll            machine binary; deprioritized
sources/windows/CredProvCommonCore.dll   credential-provider common core
sources/windows/CredProvDataModel.dll    primary reverse-engineering target
```

---

## Immediate task for the next coding agent

**Do not start by implementing the final behavior.**

Build a minimal diagnostic Windhawk mod targeting `LogonUI.exe` that:

1. logs module availability,
2. detects natural loading of `CredProvDataModel.dll`,
3. enumerates its public PDB symbols,
4. filters/logs likely credential-selection symbols,
5. performs no authentication or selection modification.

Then use the resulting symbol list/runtime trace to choose the narrowest hook for making PIN the initial selected method while retaining fingerprint authentication.
