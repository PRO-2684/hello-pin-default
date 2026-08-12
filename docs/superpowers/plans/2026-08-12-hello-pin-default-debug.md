# Hello PIN Default Diagnostic Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a read-only Windhawk mod for `LogonUI.exe` that observes natural credential-model module loading and logs relevant public symbols from `CredProvDataModel.dll`.

**Architecture:** A single Windhawk source file starts one bounded worker thread. The worker polls `GetModuleHandleW` without loading authentication DLLs, logs each target module when naturally present, and enumerates/filter-logs symbols once `CredProvDataModel.dll` appears; unload signals and joins the worker.

**Tech Stack:** C++23, Win32, Windhawk mod API

## Global Constraints

- Target only `LogonUI.exe`.
- Do not load authentication DLLs, install hooks, or modify credential-selection behavior.
- Do not log credential contents, serialization buffers, PINs, or other secrets.
- Fail open if the worker cannot start or symbols cannot be enumerated.
- Keep worker lifetime bounded and unload-safe.

---

### Task 1: Diagnostic Windhawk mod

**Files:**
- Create: `hello-pin-default-debug.wh.cpp`

**Interfaces:**
- Consumes: Win32 `GetModuleHandleW`, event/thread APIs, and Windhawk `Wh_FindFirstSymbol`, `Wh_FindNextSymbol`, `Wh_FindCloseSymbol`, `Wh_Log`.
- Produces: Windhawk log entries for module availability and filtered `CredProvDataModel.dll` symbols; no behavioral hooks.

- [x] **Step 1: Add mod metadata and readme**

Create a Windhawk mod with ID `hello-pin-default-debug`, version `0.1`, and `@include LogonUI.exe`. Document the required Windhawk process inclusion and that the mod is diagnostic/read-only.

- [x] **Step 2: Implement case-insensitive symbol filtering**

Use a local wide-string substring helper and a fixed keyword list containing `Credential`, `Provider`, `Select`, `Selected`, `Default`, `Fallback`, `Selector`, `Method`, `Bucket`, `User`, `PIN`, `NGC`, and `Bio`. Match both undecorated and decorated names.

- [x] **Step 3: Implement symbol enumeration**

Call `Wh_FindFirstSymbol(module, nullptr, &findData)`, iterate with `Wh_FindNextSymbol`, log matching symbol names and addresses, always close the search handle, and log totals. If enumeration cannot start, log and leave LogonUI behavior untouched.

- [x] **Step 4: Implement bounded natural-load monitoring**

Start one worker in `Wh_ModInit`. For at most 120 seconds, poll `authui.dll`, `CredProvCommonCore.dll`, and `CredProvDataModel.dll` every 250 milliseconds using only `GetModuleHandleW`. Log each module once; enumerate symbols once the data-model DLL appears. Exit early after enumeration or when unload signals the stop event.

- [x] **Step 5: Implement unload-safe cleanup**

In `Wh_ModUninit`, signal the stop event, join the worker, close both handles, and log shutdown. If event or thread creation fails, clean up and return `TRUE` so authentication remains unaffected.

- [x] **Step 6: Verify source statically**

Check that the source contains no `LoadLibrary`, no hook registration, no credential-data access, and only targets `LogonUI.exe`. If a Windhawk compiler is locally available, compile the mod and require a clean result; otherwise report that runtime compilation must be done in Windhawk.
