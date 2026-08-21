# Make Windows Hello PIN the default

A small experimental [Windhawk](https://windhawk.net/) mod that selects the Windows Hello PIN credential by default on the Windows lock/sign-in screen. Fingerprint authentication remains active in parallel.

```text
Start typing       -> input goes directly to the PIN field
Touch fingerprint  -> biometric authentication still works
```

## Why

You opened your laptop and pressed your finger on the sensor. It failed to recognize. You start typing your PIN, but nothing happens because Windows still has the fingerprint credential selected. To continue, you must:

1. open "Sign-in options",
2. choose PIN,
3. type it.

That fallback should not require extra clicks. With this mod, the PIN field is ready from the start, so typing works immediately. The fingerprint reader stays active too, so a successful touch still signs you in normally.

## Install

1. In Windhawk's advanced settings, add `LogonUI.exe` to the process inclusion list.
2. Create a new local mod and replace its source with [`hello-pin-default.wh.cpp`](hello-pin-default.wh.cpp).
3. Compile and enable the mod.
4. Lock Windows with <kbd>Win</kbd>+<kbd>L</kbd> and test both PIN entry and fingerprint unlock before relying on it at sign-in or boot.

## Compatibility and status

Validated on:

- Windows `10.0.26200.9168`
- `CredProvDataModel.dll` `10.0.26100.8972`
- Windhawk `1.7.3` x86-64
- Lock/unlock using both PIN typing and fingerprint authentication

The mod relies on a private Windows function resolved through Microsoft's public symbols. If the DLL or symbol changes, it leaves Windows' original selection unchanged. Fresh sign-in, cold boot, failure paths, and multi-user scenarios still need broader testing.

The current prototype assumes the selected user has an NGC PIN credential. It does not yet check NGC availability per user, so multi-user systems require extra caution.

## How it works

Windows returned the WinBio credential provider as the default for the tested known-user tile. Runtime tracing established that the PIN credential uses the NGC provider. The mod hooks only the default-provider lookup, calls Windows' original implementation first, and makes this exact substitution:

```text
WinBio {BEC09223-B018-416D-A0AC-523971B639F5}
   -> NGC {D6886603-9D2F-4EB2-B667-1971041FA96B}
```

Every failed, null, or non-WinBio result is returned unchanged. The mod does not disable fingerprint, filter credential providers, intercept keystrokes, or inspect credential contents.

## Roll back

Disable or remove the mod in Windhawk. No registry policy or credential provider configuration is changed.

Development details and research findings are in [`DEV.md`](DEV.md).

## License

[MIT](../../LICENSE)
