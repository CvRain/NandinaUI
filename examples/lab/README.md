# Nandina Lab (P2.4 example)

One executable, several routes, one resource package. This is the end-to-end
application example: it is intentionally small, but it exercises the parts a
real desktop app depends on instead of only rendering isolated components.

| Route | What it exercises |
| --- | --- |
| Sign in / Register | Single-line inputs, reactive validation, disabled action state, local session store |
| Notes | Keyed list reconciliation, selection, `TextArea` editing, long text and scrolling |
| Preview | `res://` image loading from the generated resource key header |
| Focus | Frame-driven timer (`on_process`), pause/resume/reset and phase switching |

The password fields are visible on purpose: this alpha has no password-masking
API. Credentials stay in memory; the example has no network or database
persistence.

## Build and run

```bash
# examples/lab participates in normal UI builds by default
meson setup buildDir --wrap-mode=nodownload
meson compile -C buildDir nandina_lab
./buildDir/examples/lab/nandina_lab
```

`examples` defaults to true for UI builds and can be disabled with `-Dexamples=false`.
The target is a main-project example; downstream
subproject consumers do not build it. It requires the resource toolchain; use
`-Dexternal_nanres=/path/to/nanres` when `-Dresource_tools=false`.

The Meson target:

1. runs `nanres_build_helper.py build` into
   `buildDir/examples/lab/resources/`;
2. runs `nanres_build_helper.py keys` into
   `buildDir/examples/lab/resource_keys.hpp`;
3. makes both real inputs of the executable (`link_depends` for the package,
   generated header in `sources` for the compile).

A targeted `meson compile -C buildDir nandina_lab` therefore creates the
package and header before linking. The app resolves the package beside its
executable, so a portable staging directory needs no build-tree paths.

## Routes and test hooks

Normal use is mouse/keyboard. For automated smoke tests:

| Variable | Effect |
| --- | --- |
| `NANDINA_LAB_START_ROUTE=notes\|login\|focus\|preview` | Initial route |
| `NANDINA_LAB_SMOKE_SECONDS=<seconds>` | Request close after roughly that many seconds |

The smoke hook lives in the example window; it is not a framework API.

## Checks

```bash
meson test -C buildDir lab-store example-lab-build example-lab-run example-lab-incremental --print-errorlogs
```

- `example-lab-build` checks the package keys/media types and the generated
  constants.
- `example-lab-incremental` copies the policy, adds and removes a resource,
  checks the no-change rewrite rule, and verifies a failed policy leaves the
  previous package untouched.
- `example-lab-run` first renders Preview from the build-tree package (the
  `resource-location.json` path), then stages the executable and package into a
  temporary tree, moves it, and runs `login`, `notes`, `focus` and `preview`
  once each. Every run must exit 0 with no page error, resource-lookup failure or
  texture decode/upload warning in the log. It reports SKIP when no display is
  available.

## Manual acceptance evidence

`docs/images/lab/` keeps the screenshots from the interactive acceptance pass:

| Image | What it shows |
| --- | --- |
| `notes-signed-in.png` | Sign in → Notes with the session line and the keyed list |
| `notes-editor.png` | A long note selected, editor content and the action row |
| `notes-dialog.png` | Delete confirmation overlay above the page |
| `focus-timer.png` | Focus countdown running after Start |
| `preview.png` | Cover and logo loaded through `res://` from the package |

The pass used synthetic pointer events (ydotool; absolute coordinates scaled by
the local 2× factor) plus screenshots. Keyboard text injection is still manual —
form submission is exercised through the "Use demo" prefill path.

Portable staging manually:

```bash
python3 tools/portable_stage.py \
  --executable buildDir/examples/lab/nandina_lab \
  --resources buildDir/examples/lab/resources \
  --output /tmp/nandina-lab-stage
mv /tmp/nandina-lab-stage /tmp/nandina-lab-stage-moved
# Use DISPLAY=:0 for X11/XWayland, or keep the session's WAYLAND_DISPLAY for Wayland.
DISPLAY=:0 NANDINA_LAB_START_ROUTE=preview \
  NANDINA_LAB_SMOKE_SECONDS=1 /tmp/nandina-lab-stage-moved/nandina_lab
```

## Known boundaries

- No page golden for the lab example; interactive acceptance is still manual.
  Showcase remains the component-level visual acceptance surface.
- Notes, account state and the focus timer are in memory. Navigation destroys
  the Focus page, so the timer stops when leaving the route.
- Password masking is not implemented; the form says so at the call site.
- The generated key header is a build input, not an installed SDK header.
