# Development notes

## Repository layout

```text
hello-pin-default.wh.cpp              working behavioral mod
tools/windhawk-api-stub.h              local syntax-check declarations
research/hello-pin-default-debug.wh.cpp passive diagnostic mod
research/logs/                         captured runtime evidence
research/archive/                      original binaries, sources, and plans
```

Only `README.md` is user-facing. This file records implementation, validation,
and maintenance details.

## Behavior and architecture

The target behavior is:

```text
keyboard input      -> PIN field
fingerprint sensor  -> biometric authentication remains active
```

The mod targets only `LogonUI.exe`. A bounded worker waits for the natural load
of `CredProvDataModel.dll`; it never calls `LoadLibrary` for an authentication
DLL. Once present, the worker resolves this exact public-PDB symbol:

```text
CUserData::v_GetDefaultSelectedProviderId(GUID*)
?v_GetDefaultSelectedProviderId@CUserData@@MEAAJPEAU_GUID@@@Z
```

The hook calls the original function first. It replaces the successful output
only when it exactly equals the WinBio provider:

```text
WinBio  {BEC09223-B018-416D-A0AC-523971B639F5}
NGC     {D6886603-9D2F-4EB2-B667-1971041FA96B}
```

All other outputs and HRESULTs pass through unchanged. Hook resolution,
registration, and application failures leave authentication behavior untouched.

## Research findings

### Symbol discovery

The initial diagnostic enumerated 7,502 symbols from
`CredProvDataModel.dll`; 3,693 matched a broad credential-selection filter.
The full output is
[`research/logs/2026-08-12-16-25.txt`](../research/logs/2026-08-12-16-25.txt).

Passive hooks then traced:

```text
CUserData::v_GetDefaultSelectedProviderId(GUID*)
CCredProvDataModel::_SetDefaultSelection(UINT, bool, CREDENTIALSCHANGED_STATE, bool)
CCredentialData::SelectAsync(TILE_SELECTION_FLAGS)
CCredProvDataModel::_SetSelectedBucket(ICredentialBucket*, bool*)
CCredentialData::get_ProviderId(GUID*)
CCredentialData::GetClsid(GUID*)
```

### Initial selection

The focused selection trace is
[`research/logs/2026-08-12-16-37.txt`](../research/logs/2026-08-12-16-37.txt).
Windows returned WinBio from `v_GetDefaultSelectedProviderId`, then selected
the fingerprint credential with `SelectAsync(0x01)`.

Manual transitions consistently appeared as:

```text
current credential  SelectAsync(0x28)
new credential      SelectAsync(0x21)
```

These flag meanings are inferred from sequence position; their individual bits
have not been decoded. `_SetSelectedBucket` was not observed on this path.

### Provider identity

The provider trace is
[`research/logs/2026-08-12-16-44.txt`](../research/logs/2026-08-12-16-44.txt).
It correlated the runtime objects with:

```text
Fingerprint credential  {BEC09223-B018-416D-A0AC-523971B639F5}  WinBio
PIN credential          {D6886603-9D2F-4EB2-B667-1971041FA96B}  NGC
```

The `get_ProviderId` interface pointer was consistently `0x18` lower than the
corresponding `SelectAsync` pointer, compatible with COM interface-pointer
adjustment.

The machine also registered `PINLogonProvider`
`{CB82EA12-9F71-446D-89E1-8D0924E1256E}`, but the runtime PIN credential was
conclusively associated with NGC, not that provider.

### Behavioral validation

The successful prototype trace is
[`research/logs/2026-08-12-16-50.txt`](../research/logs/2026-08-12-16-50.txt).
It shows the hook installed and the exact WinBio-to-NGC substitution executed.
Manual testing confirmed that PIN typing and fingerprint authentication both
worked seamlessly while PIN remained selected.

## Build checks

Windhawk compiles mods in the application. A local syntax-only check is also
available through the retained minimal API stub:

```powershell
g++ -std=c++23 -Wall -Wextra -Werror -fsyntax-only `
  -include tools/windhawk-api-stub.h hello-pin-default.wh.cpp

g++ -std=c++23 -Wall -Wextra -Werror -fsyntax-only `
  -include tools/windhawk-api-stub.h research/hello-pin-default-debug.wh.cpp
```

The stub is deliberately incomplete and does not replace compilation or runtime
testing in Windhawk.

## Test matrix

| Scenario | Status | Expected result |
|---|---|---|
| Lock → type PIN | Passed | PIN input receives typing immediately |
| Lock → fingerprint | Passed | Fingerprint unlock succeeds while PIN is selected |
| Fingerprint/PIN switching | Passed during diagnostics | Both methods remain selectable |
| Sign out → sign in | Pending | Same behavior on a fresh session |
| Cold boot → sign in | Pending | PIN defaults without affecting biometric auth |
| Bad fingerprint | Pending | PIN remains directly usable |
| Wrong PIN | Pending | Normal Windows behavior remains intact |
| Multiple users | Pending | Other users retain usable sign-in options; no lockout |

## Safety rules

- Never run the diagnostic and behavioral mods together.
- Never disable or filter the WinBio provider.
- Never inspect PIN fields, serialization buffers, or credential secrets.
- Prefer exact public-PDB symbols over offsets.
- Keep the original-call path intact and fail open on unknown builds.
- The current prototype does not verify NGC availability for each selected
  user; treat multi-user use as unvalidated.
- Test lock/unlock before relying on the mod at boot.
- Disable the mod for immediate rollback.
