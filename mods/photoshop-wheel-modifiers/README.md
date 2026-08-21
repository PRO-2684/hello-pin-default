# Photoshop wheel modifier remap

An experimental [Windhawk](https://windhawk.net/) mod that gives Photoshop's canvas wheel controls a more conventional modifier layout.

| Input                    | Action               |
| ------------------------ | -------------------- |
| Wheel                    | Vertical scroll      |
| <kbd>Ctrl</kbd> + wheel  | Zoom                 |
| <kbd>Shift</kbd> + wheel | Horizontal scroll    |
| <kbd>Alt</kbd> + wheel   | Fast vertical scroll |

Modifiers compose naturally. For example, <kbd>Alt</kbd>+<kbd>Shift</kbd>+wheel performs fast horizontal scrolling, while <kbd>Alt</kbd>+<kbd>Ctrl</kbd>+wheel performs fast zooming.

## Before enabling

In Photoshop, keep **Preferences > Tools > Zoom With Scroll Wheel** disabled. Then follow the collection's [installation guide](../../README.md#install-a-mod) using [`photoshop-wheel-modifiers.wh.cpp`](photoshop-wheel-modifiers.wh.cpp).

## Compatibility and status

The mod is experimental and targets `Photoshop.exe`. It currently applies the remapping only to canvas wheel messages sent to Photoshop's `PSViewC` window class, leaving wheel input over other interface elements unchanged.

Diagnostic logging is currently enabled, so the Windhawk log will contain wheel-message and modifier-state details while the mod is active.

## How it works

Photoshop already provides the desired actions under different modifiers. The mod intercepts wheel messages as Photoshop retrieves them and temporarily presents this remapping to Photoshop:

```text
Physical Ctrl   -> Photoshop Alt
Physical Shift  -> Photoshop Ctrl
Physical Alt    -> Photoshop Shift
```

The override is local to the message-processing thread and is cleared before the next message is retrieved. If hook installation fails, Windhawk unloads the mod and Photoshop behavior remains unchanged.

## Roll back

Disable or remove the mod in Windhawk. It does not change Photoshop preferences or configuration files.
