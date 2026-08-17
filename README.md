# Inventory Sorter

Standalone Levi Launchroid / Preloader Android native QoL mod.

## Behavior

- Adds a **Sort Inventory** button to the module configuration.
- Sorts the player inventory by item ID and, optionally, damage value.
- Optionally sorts the hotbar independently.
- Uses Minecraft native container-controller functions for stack inspection and slot selection; it does not write raw inventory memory or create custom packets.
- Processes swaps across render frames instead of sending one large burst of actions.

## Target

The included signatures are matched to the uploaded `libminecraftpe.so` and are intentionally limited to the functions required by this mod. This is not a universal binary for arbitrary Bedrock versions.

## Build

Use the included GitHub Actions workflow or:

```text
xmake f -y -p android -a arm64-v8a -m release --ndk=<NDK_PATH>
xmake -y
```

The Android build produces `InventorySorter.levipack`.
