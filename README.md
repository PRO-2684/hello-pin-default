# Windhawk mods

A personal collection of experimental [Windhawk](https://windhawk.net/) mods.

## Mods

| Mod                                                           | Description                                                                           | Status          |
| ------------------------------------------------------------- | ------------------------------------------------------------------------------------- | --------------- |
| [Make Windows Hello PIN the default](mods/hello-pin-default/) | Makes the PIN field ready for typing while keeping fingerprint authentication active. | 🟡 Experimental |
| [Photoshop wheel modifier remap](mods/photoshop-wheel-modifiers/) | Makes Ctrl+wheel zoom, Shift+wheel scroll horizontally, and Alt+wheel scroll quickly. | 🟡 Experimental |

Each mod directory contains the self-contained `.wh.cpp` source used by Windhawk and its user documentation. Some mods also include development notes.

## Install a mod

1. Read the mod's README for requirements and compatibility notes.
2. In Windhawk, create a new local mod.
3. Replace the generated source with the mod's `.wh.cpp` file.
4. Compile and enable the mod.
5. Restart the target application if the mod doesn't take effect immediately.

To remove a mod, disable or delete it in Windhawk. Check its README for any additional rollback instructions.

## License

[MIT](LICENSE)
