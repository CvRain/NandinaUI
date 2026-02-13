# NandinaUI Docs (English)

> Mainline status: V2 refactor in progress

NandinaUI is moving from V1 to V2 with a clearer direction:
- Component behavior and composition inspired by shadcn.
- UI language inspired by Svelte Skeleton.
- Catppuccin-based theming with a stronger semantic token layer.

Please read the full project overview here:
- [Root README](../README.md)

## Available modules
- Nandina.Window
- Nandina.Theme
- Nandina.Color
- Nandina.Core

## Run the example

```bash
cmake -S .. -B ../build/Debug
cmake --build ../build/Debug -j4
../build/Debug/example/NandinaExampleApp
```

## Notes
- Docs are being updated along with the V2 refactor.
- If docs and code differ, treat source code and the root README as authoritative.
