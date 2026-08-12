# Passive Selection Tracing Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace bulk symbol logging with four passive hooks that expose LogonUI's initial provider choice and subsequent credential-selection transitions.

**Architecture:** The existing bounded worker waits for `CredProvDataModel.dll`, resolves four exact decorated PDB symbols, registers pass-through hooks, then applies them once after `Wh_ModInit` has returned. Hooks log only GUIDs, flags, object addresses, HRESULTs, and change booleans.

**Tech Stack:** C++23, Win32, Windhawk mod API

## Global Constraints

- Target only `LogonUI.exe`.
- Never load authentication DLLs or change arguments, return values, provider GUIDs, buckets, or selection state.
- Never log credential contents, serialization buffers, PINs, SIDs, or other secrets.
- Require all four exact symbols; if any are absent, install no hooks.
- Fail open on resolution, registration, or application failure.

---

### Task 1: Passive selection hooks

**Files:**
- Modify: `hello-pin-default-debug.wh.cpp`

**Interfaces:**
- Consumes: four exact decorated symbols captured in `reference/2026-08-12-16-25.txt`.
- Produces: `[trace]` log records for default-provider lookup, default selection, manual selection requests, and selected-bucket changes.

- [x] Replace broad keyword filtering and bulk symbol output with exact decorated-name resolution.
- [x] Add ABI-compatible pass-through hooks for `CUserData::v_GetDefaultSelectedProviderId`, `CCredProvDataModel::_SetDefaultSelection`, `CCredentialData::SelectAsync`, and `CCredProvDataModel::_SetSelectedBucket`.
- [x] Format the returned provider GUID locally without allocating or logging user data.
- [x] Register hooks only after all symbols resolve, then call `Wh_ApplyHookOperations` once from the worker.
- [x] Preserve bounded natural-load polling and unload-safe worker cleanup.
- [x] Verify C++23 syntax with warnings as errors and statically prove all original functions are called with unchanged arguments.
