# Windows LogonUI credential-selection observations

## Goal

Make the Windows Hello PIN credential selected by default for a known user tile
while leaving fingerprint authentication available in parallel.

The intended behavior is:

```text
keyboard input      -> PIN field
fingerprint sensor  -> biometric authentication remains active
```

## Environment

- Windows: `10.0.26200.9168`
- `CredProvDataModel.dll`: `10.0.26100.8972`
- Windhawk: `1.7.3` x86-64
- Focused selection trace: `hello-pin-default-debug` version `0.2`
- Provider-identity trace prepared: version `0.3`

## Symbol discovery

The initial diagnostic run enumerated 7,502 public symbols from
`CredProvDataModel.dll`; 3,693 matched the broad credential-selection filter.
The complete output is in
[`reference/2026-08-12-16-25.txt`](../reference/2026-08-12-16-25.txt).

Four functions were selected for passive, pass-through tracing:

```text
CUserData::v_GetDefaultSelectedProviderId(GUID*)
CCredProvDataModel::_SetDefaultSelection(UINT, bool, CREDENTIALSCHANGED_STATE, bool)
CCredentialData::SelectAsync(TILE_SELECTION_FLAGS)
CCredProvDataModel::_SetSelectedBucket(ICredentialBucket*, bool*)
```

The hooks preserve all arguments and return values and do not inspect credential
contents or authentication serialization.

## Runtime observations

The focused trace is in
[`reference/2026-08-12-16-37.txt`](../reference/2026-08-12-16-37.txt).

### Initial provider

`CUserData::v_GetDefaultSelectedProviderId` returned:

```text
{BEC09223-B018-416D-A0AC-523971B639F5}
```

This is the registered WinBio Credential Provider. Windows is therefore
explicitly choosing the fingerprint provider as the default for this known user
tile; the fingerprint-first state is not merely a focus accident in the visual
layer.

### Credential object transitions

Two stable `CCredentialData` object addresses appeared during manual switching:

| Object suffix | Observed role | Evidence |
|---|---|---|
| `...6C88` | Fingerprint | Selected immediately after the WinBio provider was returned as default |
| `...9868` | PIN | Entered when the UI was manually switched from fingerprint to PIN |

The observed sequence was:

| Time | Object | Flags | Interpretation |
|---|---:|---:|---|
| `16:37:15.968` | `...6C88` | `0x01` | Initial fingerprint selection |
| `16:37:17.542` | `...6C88` | `0x01` | Repeated selection/refresh of fingerprint |
| `16:37:19.192` | `...6C88` | `0x28` | Fingerprint leaving selection |
| `16:37:19.208` | `...9868` | `0x21` | PIN entering selection |
| `16:37:19.889` | `...9868` | `0x28` | PIN leaving selection |
| `16:37:19.889` | `...6C88` | `0x21` | Fingerprint entering selection |
| `16:37:20.673` | `...6C88` | `0x28` | Fingerprint leaving selection |
| `16:37:20.673` | `...9868` | `0x21` | PIN entering selection |

The meanings assigned to `0x01`, `0x21`, and `0x28` are inferences from their
position in the transition sequence; their individual flag bits have not yet
been decoded.

### Default-selection callback

`CCredProvDataModel::_SetDefaultSelection` ran at `16:37:22.780` with:

```text
credentialCount=17
userSelected=true
credentialsChangedState=0
unknownFlag=false
```

Because it ran after the manual transitions and reported `userSelected=true`,
this invocation appears to preserve or process an explicit user selection. It
does not currently look like the source of the initial fingerprint choice.

### Selected-bucket callback

`CCredProvDataModel::_SetSelectedBucket` was not observed during this scenario.
It is not a useful primary hook for this path unless a later scenario invokes
it.

## Provider-ID caveat

The machine registers both of these PIN-related providers:

```text
PINLogonProvider          {CB82EA12-9F71-446D-89E1-8D0924E1256E}
NGC Credential Provider  {D6886603-9D2F-4EB2-B667-1971041FA96B}
```

The runtime PIN credential must be identified before either GUID is used in a
behavioral hook. The symbol catalog contains two suitable passive getters:

```text
CCredentialData::get_ProviderId(GUID*)
CCredentialData::GetClsid(GUID*)
```

Diagnostic version `0.3` traces these getters. The next capture will correlate
their `this` pointers with the known PIN object (`...9868`).

## Current working hypothesis

If the PIN credential's runtime provider GUID is identified, the smallest
behavioral change is likely to alter only the successful output of
`CUserData::v_GetDefaultSelectedProviderId`:

```text
WinBio provider GUID -> runtime PIN provider GUID
```

This hypothesis is not yet implemented. It must first be validated by the
provider-ID trace, and the behavioral prototype must confirm that fingerprint
authentication remains active while PIN is selected.
