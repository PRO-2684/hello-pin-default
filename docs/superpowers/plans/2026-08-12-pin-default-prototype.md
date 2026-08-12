# PIN Default Prototype Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a separate Windhawk prototype that changes the known-user default provider from WinBio fingerprint to the observed NGC PIN provider.

**Architecture:** A standalone mod waits for naturally loaded `CredProvDataModel.dll`, resolves only `CUserData::v_GetDefaultSelectedProviderId(GUID*)`, and installs one hook. The hook calls the original first and substitutes NGC only when the successful output exactly equals WinBio.

**Tech Stack:** C++23, Win32, Windhawk mod API

## Global Constraints

- Keep the diagnostic mod unchanged and use a distinct mod ID.
- Never load authentication DLLs or touch credential contents.
- Preserve every non-WinBio GUID and every failed/null output unchanged.
- Warn users not to enable the diagnostic and prototype mods together.
- Fail open if the module, symbol, hook registration, or hook application is unavailable.

---

### Task 1: Behavioral prototype

**Files:**
- Create: `hello-pin-default.wh.cpp`
- Create: `tools/windhawk-api-stub.h`
- Modify: `docs/credential-selection-observations.md`
- Add: `reference/2026-08-12-16-44.txt`

- [x] Implement natural-load monitoring and exact symbol resolution.
- [x] Implement the single conditional WinBio-to-NGC substitution hook.
- [x] Add rollback, coexistence, and prototype-scope warnings to the readme.
- [x] Record the conclusive provider-identity trace and prototype decision.
- [x] Keep a reusable minimal Windhawk API stub and verify C++23 syntax with:
  `g++ -std=c++23 -Wall -Wextra -Werror -fsyntax-only -include tools/windhawk-api-stub.h hello-pin-default.wh.cpp`.
- [x] Verify one hook only and preservation of all other outputs.
