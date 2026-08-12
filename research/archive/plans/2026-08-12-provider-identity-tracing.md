# Provider Identity Tracing Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Correlate each selected `CCredentialData` object with its runtime provider GUID and CLSID.

**Architecture:** Extend the existing exact-symbol resolver and passive hook set with `CCredentialData::get_ProviderId(GUID*)` and `CCredentialData::GetClsid(GUID*)`. Both hooks call the original unchanged, then log the object address, HRESULT, and successful GUID output.

**Tech Stack:** C++23, Win32, Windhawk mod API

## Global Constraints

- Target only `LogonUI.exe` and naturally loaded `CredProvDataModel.dll`.
- Change no arguments, GUID outputs, return values, or authentication state.
- Log no credential contents or authentication serialization.
- Require all six exact symbols before applying any hooks.

---

### Task 1: Provider identity hooks

**Files:**
- Modify: `hello-pin-default-debug.wh.cpp`
- Modify: `docs/credential-selection-observations.md`

- [x] Add exact resolution and pass-through hooks for `get_ProviderId` and `GetClsid`.
- [x] Log each getter's `this` pointer and successful GUID output.
- [x] Update mod metadata/readme and research status.
- [x] Verify C++23 syntax with warnings as errors and all six unchanged original calls.
