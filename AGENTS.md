# Repository Guidelines

> Fast orientation for AI assistants. Authoritative contracts live in `docs/references/`
> (start at `README.md`, `development_loop.md`, `coding_conventions.md`); this file is the
> working summary. Source and gate output override stale prose.

## Project Overview

NandinaUI is a C++26 native desktop GUI framework built with Meson. It provides a retained-mode
scene tree, a signal-based reactive layer, a declarative builder DSL, themes/design tokens, text
shaping (FreeType + HarfBuzz + FriBidi), resource packaging (`nanres`), and a Router/Page
application shell. Version `0.1.0-alpha.1`.

- Formally supported: **Linux desktop source integration** (X11/Wayland, SDL3 backend).
- Experimental: Windows (MSYS2 UCRT64). Unsupported: macOS.
- Not a Node/Bun project; there is no JavaScript runtime or package manager.

### Current milestone state (2026-11-23)

- **Phase state**: P1/P2 closed; P3 body delivered (P3.3b2b platform IME / standalone Xorg /
  Windows candidate-window acceptance still open); P4 in progress. Authoritative per-batch
  status, contracts and evidence live in `docs/references/` — start at
  `development_roadmap.md`, `project_status.md`, `component_roadmap.md`; `stories/` keeps
  root-cause narratives.
- **Data-view contract (§9 v3.1, `data_components_and_virtualization.md`)**: non-template
  `DataTable` / `ListView` / `TreeView` over `IRowModel` + `DataTableColumn` +
  `IItemDelegate` + explicit selection; `ForEach` stays as the declarative full-list region.
  Batches 1A–6 committed (`VirtualListView` retired; `TreeView` v1 with `ITreeModel`, view-owned
  expansion, uniform-row virtualization, `SelectionModel<RowId>`, framework-drawn expander; §10).
  Author-ergonomics sub-batch 1 committed (9 headers promoted from `widget/internal/` to
  `widget/`, themed `TextItemDelegate` selection/current-cell painting, owner-release-safe
  `DataTable` `selection_changed`/`sort_changed`, `ComponentTraits` + catalog entries; §11).
- **Disclosure, DSL, overlay & layout batches committed**:
  - **Batch 7 (Accordion)**: composes `Collapsible` sections (`disclosure_components.md` §7),
    single/multiple open state, Up/Down/Home/End roaming, transactional `set_sections`.
  - **Layout DSL shorthand sub-batch 2**: `ui.wrap()` / `authoring::wrap()`, multi- and
    single-child factory shorthands, `child()` accessors on `Padding`/`Surface`/`Center`/
    `Expanded`/`FlexItem`, `fill` vs `Expanded` notes (`authoring_dsl_and_content_measurement.md`
    §§7, 19).
  - **Batch 8 (Sheet v1)**: edge-docked modal drawer (`SheetSide`, `OverlayHost`/`DismissLayer`/
    `FocusScope`/`DialogRecipe`, header/content/footer slots, `set_extent`, owner-release-safe
    `closed`; `dialog_migration.md`).
  - **Batch 9 (AspectRatio v1 + Resizable v1)**: `AspectRatio` (`width/height` inscription,
    `LayoutAlignment`, `content_rect()`) and `Resizable` (ratio-driven two-pane split, min-extent
    clamping/fallback, drag without jump, double-click reset, keyboard handle with `Role::slider`,
    owner-release-safe `ratio_changed`), both with DSL shorthands + catalog
    (`authoring_dsl_and_content_measurement.md` §20; `aspect-ratio` 4/53, `resizable` 8/135).
- **Drag-resize & text-shaping optimization (committed, `text_shaping_cost.md` §13)**: (1) `NanWindow::poll_and_dispatch_input()` coalesces adjacent same-frame
  `MouseMoveEvent`s while preserving ordering across other events; (2) `Resizable::set_ratio()`
  defers pane layout during `FramePhase::input` and `Resizable`/`Expanded` short-circuit tight
  two-axis `on_measure()`; (3) `Text` caches width-invariant unwrapped results, ignores
  `min_width`/`min_height`/`max_height` in shaping keys, skips redundant same-slot copies, and
  clears cache on backend switches; (4) `FreeTypeFontFace` caches `active_pixel_height` and
  `HarfBuzzTextLayoutBackend` reuses `scratch_buffer`, tracks per-slot `hb_ft_font_changed`,
  fast-paths single-codepoint fallback (`single_cp_font_cache_` + `glyph_index`), uses
  `next_grapheme_boundary()` + flat cluster vectors, and skips FriBidi for pure-LTR text.
  `playground --resize-bench` (Debug `-O0`) pure drag overhead: +12.28 → +3.82 ms (1 move/frame,
  3.2×), +95.64 → +3.55 ms (10 moves/frame, 26.9×); 7/7 Linux CPU page goldens PASS.
- **Page-switch stall & live window-resize smoothness (committed,
  `text_shaping_cost.md` §14, `data_components_and_virtualization.md` §9.9.2)**: (1)
  `ListView`/`DataTable`/`TreeView` with `fill`/`percent` height return `min_height` under
  unbounded (`loose()`) **pre-layout** measurement (`get_tree() == nullptr` or
  `!layout_pass_active()`, covering detached wrappers and in-tree `on_ready()` relayouts),
  preventing full-model materialization; a real layout pass keeps the existing
  content-height semantics for unbounded parents (`ScrollView`/`Stack`), so viewports do not
  collapse to 0 rows; (2) `ResolvedStyleContext::operator==` lets
  `_resolve_style_context()` prune unchanged subtrees (this is what removes the
  $O(\text{depth} \times N)$ repeat work), `_propagate_enter_tree()` now resolves only the
  current node and relies on the top-down child walk, and `remove_and_delete()` skips detached
  style resolution (`remove_child()` still resets); (3) `default_design_system()` caches its
  baseline once and `design_system_shared_from_theme()` reuses immutable 8-slot snapshots across
  33 widget files / 67 construction/theme sites; (4) `NanWindow::tick()` re-samples
  `screen_size()`/`drawable_size()` after input polling and before `layout_root()` (removing the
  stale-size tick ordering; mocked by `[app][resize][timing]`, real-window smoothness still
  needs manual acceptance), treats zero-sized (minimized/zero-height) windows as a skipped
  layout/render frame instead of throwing, `SdlPlatformWindow` also handles
  `SDL_EVENT_WINDOW_RESIZED` (defensive; fixed SDL 3.4.18 already emits
  `PIXEL_SIZE_CHANGED` on resize), and `SdlRenderDevice::SdfPath` defers evicted
  `SDL_GPURenderState` destruction to `begin_frame()`/`~SdfPath()` (reduces in-frame
  state-destruction flushes; local manual observation only, no automated GPU gate, `cached`
  still counts cache entries ≤256). `playground --self-test` (local Debug `-O0`,
  `SDL_VIDEODRIVER=dummy`, temporary untracked instrumentation — not CI evidence): `page=data`
  entry 704.9 → 61.3 ms cold (11.5×) / 656.1 → 33.1 ms warm (19.8×); leaving `page=data`
  173.5 → 30.7 ms (5.7×). Verification matrix (2026-11-23, final tree): main suite 146 OK /
  0 FAIL; ASan 121 OK +1 skip; no-RTTI 123 OK; no-SQLite 118 OK +1 skip; pure tools 8/8;
  `release_check --ci` 8/8; Linux CPU 7-page golden 7/7 PASS (`deterministic=true`).
- **Linux desktop settings & cross-platform OS preferences bridge (committed 2026-11-24; `typography_scale.md` §8)**: (1) `SystemPreferencesSnapshot` adds `appearance` (`ColorAppearance`) and `reduced_motion` (`bool`) plus pure validators `text_scale_from_factor()` (`[0.5, 3.0]`) and `text_scale_from_dpi()`; (2) `NanApplication` forwards all three OS signals into `ThemeManager::set_system_preferences()` (batched in one call) and `set_text_scaling()` with rollback on `require_idle()` throw and exception-safe `noexcept` `detach_system_preferences()`, while `NanWindow` detaches auto-attached platform sources across `close()`, `~NanWindow()`, and failed `open()` before `platform_->destroy()`; (3) `SdlPlatformWindow` wires `SdlSystemPreferencesSource` (event-driven `SDL_GetSystemTheme()` + `SDL_EVENT_SYSTEM_THEME_CHANGED`, cross-platform) over platform backends (`LinuxSystemPreferencesSource` on Linux: XDG Desktop Portal `busctl` → `gsettings` / KDE `kcmfonts`/`kdeglobals` / GTK `settings.ini` with last-wins INI + UTF-8 BOM, 150 ms bounded command timeout, 500 ms `stat()`-gated polling with zero steady-state subprocesses including headless/no-`HOME`, zero new build deps; Windows keeps registry text scale + gains SDL3 appearance, pending Windows re-verification). Default font and 28 page goldens are untouched (`page_probe` bypasses `NanWindow`); GNOME Wayland startup & runtime live switching accepted interactively; full matrix + 7/7 CPU goldens PASS.
- **Default palette desktop-contrast refinement & `butter` clay-card redesign (committed 2026-11-25; `design_tokens.md`)**: (1) Default light/dark palettes split previously identical `muted`/`secondary`/`accent` into 3 progressive neutral/cool-slate tiers (light `0.946`/`0.916`/`0.882`, dark `0.248`/`0.296`/`0.342`) and sharpen `border`/`ring` while preserving achromatic `primary`, fixing washed-out Badges and selected rows (Linux 14 CPU+SDF page goldens recaptured & approved 2026-11-25; Windows 14 page goldens untouched pending Windows recapture); (2) `ThemeFamilyDefinition` adds optional `light_palette`/`dark_palette` (`SemanticColorSpec`) and `button_rules`/`badge_rules`/`dialog_rules`; `butter` is redesigned as Sunlit Butter Clay / Warm Cocoa Clay (`radius 8/12/20`, `border 1.25/2.25`, pure-white clay cards over `#FCF7EA` sunlit buttercream with restrained `offset_y=3/spread=8` espresso shadow, vibrant `#E0640D` orange with dark ink, clay-tile `outlined` & pastel `tonal` buttons, stickered Badges, warm cocoa scrim); (3) `theme_palette_contrast_tests` now enforces `default_contrast_profile()` + badge/selection/border separation floors across `default` and built-in families.
- **`AnchorCanvas` arranged-container composition & non-cyclic `content` sizing (committed 2026-11-25; `anchors.md` §5.1/§5.3)**: (1) `NanControl::child_axis_assignment()` (`ChildAxisAssignment { none, pass_through, assigned }`) declares parent-assigned child axes across `Column`/`Row`/`Flex`/`Stack`/`Padding`/`Card`/`Expanded`/`FlexItem`/`PointerArea`/`IfRegion`/`FocusScope`/`Resizable`/`AspectRatio`/`ScrollView`/`AnchorCanvas` without upward `scene → widget` coupling, `NanControl::layout_to()` restores ancestor layout dirtiness on `on_layout()` exceptions, and `Expanded::on_measure()` preserves incoming non-zero min constraints (matching `FlexItem`); (2) `AnchorCanvas` defers eager container `ProvisionalRelayoutScope` passes without clobbering committed canvas sizes, composes cleanly via `Expanded`/`fill`/`percent` inside arranged containers, and solves non-cyclic `content` axes (`max(0, child_far_edge)`) whenever no child references far/center parent anchors or percentages on that axis (cyclic parent dependencies and unbounded `fill`/`percent` deterministically fail; `set_child_anchors()` validates against active axis mode); 0 showcase page goldens changed; `max_width` vs tight parent (`layout_convergence_and_text_measurement.md` §7.1) remains a separate open item.
- **Baseline counts**: 146 tests (123 unit + 23 integration), 220 headers (202 experimental +
  18 internal), 28 approved page goldens (7 pages × CPU/SDF × 2 platforms), 46 public
  components; the CPU page-golden CI gate is blocking (7 pages).
- **Do not over-claim**: Windows is experimental (only real exit codes/logs count; GPU/SDF and
  GUI checks need an interactive desktop, CPU paths can use `--renderer sdl2d` over SSH);
  macOS unsupported. Open: real IME candidate placement / standalone Xorg / Windows
  interaction, Wayland fractional scaling, display hot-plug and online scale changes,
  app-level text-scale persistence & Windows appearance live re-verification, Theme Shell page and remaining
  probe/golden coverage, notebook UI visual acceptance / todo / rich text, P4.8c build-time
  measurement batching. The registration contrast gate is not wired — never claim the full
  color gate passes. P4.7 real multi-face assets are deferred (synthetic fallback is the
  default). Page goldens are platform-isolated; never copy baselines across platforms.
- **Linux verification**: the second machine is reachable via the local SSH alias
  (`nandina-linux`); session setup, golden capture/approval rules and security boundaries live
  in `docs/references/dual_platform_verification.md`. Run commits from Windows and `git pull`
  on Linux; never commit remote credentials.
- **Capability snapshot** (review records and exact evidence live in the linked docs):
  - Text/label: typography scale (`TextSize`/`Leading`/`NanTypeScale`), `TypographyRole`,
    user text scaling + cap, overflow/max-lines, box vertical alignment, and one managed text
    binding shared by Property/Label/BuildContext. See `typography_scale.md`,
    `authoring_dsl_and_content_measurement.md` §16–§17.
  - Surfaces/layout: `ui.surface()`, Card content insets, Image cover anchors (rounded corners
    do not mask children — rectangular scene clip); Table shared tracks/natural row
    heights/semantics (selection/sort/virtualization belong to DataTable); Collapsible retained
    content + Signal + focus restore; Accordion v1 composes Collapsible sections (single/multiple,
    view-owned open state, Up/Down/Home/End roaming, no new recipe). See
    `data_components_and_virtualization.md` §8, `disclosure_components.md` §7.
  - Theme: presentation/selection/catalog projections, before/after change events, scheme
    catalog + typed IDs, author color entry, 148 named colors (C++/TOML), CPU compositor +
    contrast diagnostics. See `theme_authoring_and_state.md` §5–§20.
  - Platform/fonts/pages: high-DPI logical-unit seam (`high_dpi=false` for tests/probes/
    goldens); mixed-DPI multi-monitor verified; synthetic weight/slant fallback;
    `PageContext::defer_section` progressive page mounting. See
    `high_dpi_and_multi_display.md`, `font_synthesis.md`, `page_load_responsiveness.md`.
  - Visual: Simplified-Chinese showcase, 28 approved page goldens (7 pages × CPU/SDF × 2
    platforms) with a blocking CPU gate; fixed measure-time flex distribution, GlyphAtlas
    two-axis growth and 175% dropdown-menu text regressions. See `visual_regression.md`,
    `text_shaping_cost.md` §11.
- **Data/resources and examples**: P1/P2 closed (scoped manager results, cache identity and
  reclamation, resource identity/lock, `nanres keys` header). `examples/notebook` has its SQLite
  data layer, account service, login/registration/home routes and a real workspace (group
  filter + note list + editor; `notebook-data` 228 assertions). See
  `resource_build_and_deployment.md` §8.
- The plan and exit criteria are **not** owned by this file: read
  `docs/references/development_roadmap.md` and the D5 contract
  (`resource_build_and_deployment.md` §8) before claiming a sub-batch is complete.

## Architecture & Data Flow

### Layer direction

Dependencies point downward only:

```text
app → widget → { scene, semantics, text, render } → theme → { foundation, reactive, resource, physics2d }
```

`foundation` has no upstream dependency; `reactive` is pure logic and testable without a window.
Do not introduce upward includes. Known documented debts: `theme → text` via
`theme/style_document.hpp`, and `physics2d → scene` via `physics_world2d.hpp`; move shared types
down rather than adding new debts.

### Boot and ownership

- Entry: `app::run(...)` / `app::run<PageT>(...)` in `nandina/app/nan_application.hpp`;
  manual apps subclass `NanWindow`, use `on_setup()` → `use_router(...)` + `router.start<Page>()`.
- `NanApplication` owns the reactive `Graph`, `NanStore`, `ThemeManager`, `ResourceManager`,
  `FontLoader`, `UiDispatcher`, and `BackgroundExecutor`.
- `NanWindow` owns the scene tree, `OverlayHost`, router, platform window, render device, and
  caches. Services reach widgets through `BuildContext`/`PageContext`, never globals.

### Per-frame pipeline

Order is contractual (`nandina/app/nan_window.cpp`):

```text
texture_cache.begin_frame → input → tasks (UiDispatcher) → process/on_frame
→ tree_commit (deferred mutations) → physics → reactive effects → animation
→ layout_root → semantics → render → present → dispose
```

Tree mutations (`add_child`, `remove_child`, `replace_child`, reparent) are illegal during
traversal. Use `defer_tree_mutation`, `queue_delete`, `post_layout`, or the animation host.
`layout_root()` allows one post-layout pass; do not reintroduce ancestor dirtying during layout
(`layout_size_epsilon` absorbs float noise).

### Reactive, layout, paint

- Push invalidation + pull evaluation: `Signal::set` bumps a version and invalidates subscribers;
  `Computed` recomputes lazily; `Effect`s are deferred during a window tick and flushed in the
  `reactive` phase.
- Widgets bind through `ReactiveScope::effect`, `BuildContext::bind`, `NodeBuilder::bind_scope`,
  or `Property`/`NanReactiveControl::bind`.
- Layout: `measure_layout(constraints)` then `layout_to(rect)`; `layout_pass_active()` tells
  measure output from real invalidation.
- Paint: `NanSceneTree::render(DrawContext&)` threads world transform, inherited opacity, and an
  RAII `ClipStack`. Keep `logical_to_screen` and `screen_to_physical` separate.

### Render, text, theme, resources

- `render::IRenderDevice` is a stateless primitive executor with frame bracketing and capability
  queries. `app::IPlatformWindow` provides window/input/present and `make_render_device()`;
  `set_render_device_wrapper` supports decorators such as the recording device.
- Text: `ITextLayoutBackend` + `ITextLayoutRenderer` in `TextPipeline`; real path is
  `FontLoader` → `FontPipeline` (HarfBuzz + `GlyphAtlas`) → texture upload. The deterministic
  backend is the no-font fallback.
- Theme: immutable `DesignSystem` snapshots (primitive → semantic → component recipes).
  `ThemeManager` publishes a revision; widgets re-resolve in `on_theme_changed`. Never hardcode
  component colors; use semantic tokens.
- Resources: `ResourceManager` returns `ResolvedResource`/`ResolvedStream` carrying a weak domain
  token, `MountId`, `PublishGeneration`, consistency class, record ID, and handle. Cache identity
  is `ResourceScopeIdentity` (domain + MountId + generation + ID). `non_snapshot` content
  (directory, external files) must not enter a shared cache; direct `ResourceHandle` loads bypass
  the shared font cache. Re-resolution uses `resolve(...)` and fails with `expired_scope` after a
  domain/mount/generation change.
- `FontLoader` caches by `(ResourceScopeIdentity, face_index)` with a bounded LRU
  (`FontLoaderLimits::max_entries`, default 64) and exposes `cached_entries()`;
  `invalidate(resolved)` is scoped and `invalidate_wide(id)` is explicitly wide. Eviction only
  drops associations, never destroys faces still held by callers.
- `TextureCache` keys resource images by `(ResourceScopeIdentity, ImageLoadOptions)`. Use
  `load_resolved(...)` for synchronous resolved loads. Async resource loading is two-phase: the
  worker resolves the scope, the UI thread checks the scoped cache, and only a miss submits the
  decode; a cache hit performs no decode/upload. `non_snapshot` images may be uploaded but are
  never indexed or retained. Reclamation is `sweep_expired()` (frame boundary/trim), targeted
  `invalidate(resolved)`, and the `max_retained_entries`/`max_retained_bytes` LRU; there is no
  manager-wide invalidation broadcast.

## Key Directories

| Path | Purpose |
| --- | --- |
| `nandina/foundation/` | Geometry, OKLCH color, constraints, UTF-8, JSON wrapper, logging, value motion |
| `nandina/reactive/` | Graph, signal/computed/effect/event/property/batch, `ReactiveScope` |
| `nandina/resource/` | IDs/keys/URIs, scoped manager, backends, streams, manifest/scanner |
| `nandina/theme/` | Tokens, palettes, `DesignSystem`, `ThemeManager`, `styles.toml` parsing |
| `nandina/render/` | `IRenderDevice`, draw context/clipping, `TextureCache`, image decoding |
| `nandina/text/` | Layout protocol, FreeType/HarfBuzz pipeline, glyph atlas, font caches |
| `nandina/scene/` | Node/control tree, anchors, layout kernel, animation host, overlays, clipboard |
| `nandina/semantics/` | Accessibility semantics tree |
| `nandina/widget/` | Layout primitives, `NodeBuilder`, components, painters |
| `nandina/app/` | Application, window, router/page, store, async, platform seam |
| `nandina/physics2d/` | Optional Box2D bridge (`-Dphysics2d=enabled`) |
| `tests/` | Catch2 unit tests, Python integration tests, fixtures, golden manifest |
| `tools/` | Gates, packaging, resource tools, golden/performance harnesses |
| `docs/references/` | Maintainer contracts, roadmaps, architecture, verification records |
| `docs/components/`, `docs/getting_started/`, `docs/guide/` | Component docs, tutorial, topic guides |
| `stories/` | Narrative “why” and root-cause history |
| `showcase/` | Reference application and visual acceptance target |
| `examples/notebook/` | Integrated notebook example: SQLite data layer with resource-driven migrations, then services/UI. Data-layer tests sit beside it; UI tests live in untracked `playground/` |
| `subprojects/`, `third_party/` | Vendored dependencies; do not edit or reformat |

## Development Commands

```bash
# First build (online) or offline after prewarming wraps
meson subprojects download
meson setup buildDir --wrap-mode=nodownload

# After editing meson.build / meson_options.txt
meson setup --reconfigure buildDir

# Compile and test in one chain; never test stale binaries
meson compile -C buildDir && meson test -C buildDir --print-errorlogs
meson compile -C buildDir && meson test -C buildDir --suite unit --print-errorlogs
meson test -C buildDir resource --print-errorlogs          # one Meson test name
./buildDir/tests/resource/resource_tests "[cache]"         # Catch2 tag/name filter

# Sanitizers / alternate configurations
meson compile -C buildDir-asan && \
  LSAN_OPTIONS=detect_leaks=0 meson test -C buildDir-asan --suite unit --print-errorlogs
meson compile -C buildDir-no-rtti && \
  meson test -C buildDir-no-rtti --suite unit --print-errorlogs

# Notebook example participates in normal UI builds; disable with -Dexamples=false
meson setup --reconfigure buildDir
meson compile -C buildDir nandina_examples_notebook
meson test -C buildDir notebook-data --print-errorlogs
meson compile -C buildDir-nosqlite && meson test -C buildDir-nosqlite --suite unit
meson compile -C buildDir-tools && meson test -C buildDir-tools

# Format only the files you changed (exact clang-format 23.1.1)
pip install clang-format==23.1.1
clang-format -i <changed files>
python3 tools/check_format.py

# Gates
python3 tools/check_headers.py --source . --build-dir buildDir
python3 tools/check_test_suites.py buildDir
python3 tools/release_check.py --ci --build-dir buildDir
```

Useful options: `-Dbuild_ui=false` (resource tools only; examples are skipped), `-Dresource_sqlite=disabled`,
`-Dresource_tools=false -Dexternal_nanres=/path/to/nanres`, `-Dsdl_backend=disabled
-Dshowcase=false -Dexamples=false` (headless), `-Dphysics2d=enabled`, `-Dcpp_rtti=false`.
`examples` is true by default for UI builds and can be disabled explicitly; `build_ui=false`
skips it. The notebook example needs the resource toolchain **and** `resource_sqlite=enabled`
(it stores the app database and mounts the migration package through SQLite), so headless,
tools-free and SQLite-disabled configurations pass `-Dexamples=false`. `resource_sqlite=auto`
equals enabled. `showcase` and `examples` build resource packages, so
`-Dresource_tools=false` without `-Dexternal_nanres` requires `-Dshowcase=false
-Dexamples=false` (otherwise configure stops with an explanatory error). Keep that in mind when
configuring a tools-free sanitizer or minimal build directory.

Gotchas: `meson test` does not rebuild; do not edit the workspace while the full suite runs
(`nandina-subproject-fixture` packages the tree twice and byte-compares). After target-set
changes, a stale thin archive can require deleting `buildDir/nandina/libnandina.a` or reconfiguring.

## Change Workflow (review / refine / commit)

The usual collaboration is: a maintainer writes a sub-batch in the working tree, an agent reviews
it, completes gaps, verifies, and commits. Follow this order:

1. **Read before editing**: `git status`, `git diff`, the task statement, and the owning contract
   (D5 §8 for resource work). Build and run the relevant tests first so a pre-existing failure is
   not attributed to the review.
2. **Complete the behavior, not just compilation**: check the contract's exit criteria, error
   paths, non-snapshot/expiry semantics, and consumer migrations. Add the missing boundary tests
   rather than weakening existing assertions.
3. **Red-first for every fix**: temporarily break the line that implements the behavior and confirm
   the new test fails, then restore it. Do not weaken gates to make a change pass.
4. **Run the verification matrix** (below) on the final tree, not on a stale binary.
5. **Keep the first implementation uncommitted for maintainer review.** The maintainer may add
   fixes in the same working tree; then review hidden gaps and rerun affected verification.
   Only after that review cycle is closed, commit the whole reviewable sub-batch once, with a
   conventional prefix (`feat/fix/refactor/test/docs/perf/chore`) and a body stating behavior,
   verification and remaining boundaries. Push that approved combined commit; retry transient
   network failures. Do not auto-commit the first implementation or start the next batch early.
6. **Sync docs in the same change**: roadmap status, project status, `sdk_surface.md` for
   experimental/breaking API changes, `module_dependency.md` for boundary changes, and the owning
   reference document's implementation record.

### Verification matrix

| Scope | Command / evidence |
| --- | --- |
| Main suite | `meson compile -C buildDir && meson test -C buildDir --print-errorlogs` |
| Unit only | `meson test -C buildDir --suite unit --print-errorlogs` |
| ASan/UBSan | `meson compile -C buildDir-asan && meson test -C buildDir-asan --suite unit` |
| No RTTI | `meson compile -C buildDir-no-rtti && meson test -C buildDir-no-rtti --suite unit` |
| No SQLite UI | `meson compile -C buildDir-nosqlite && meson test -C buildDir-nosqlite --suite unit` |
| Pure tools | `meson compile -C buildDir-tools && meson test -C buildDir-tools` |
| Gates | `python3 tools/release_check.py --ci --build-dir buildDir` |
| Formatting | `PATH=<pinned venv>/bin:$PATH python3 tools/check_format.py` |
| Windows | see below; only claim it with a real exit code/log |

`window-lifecycle` is skipped in the ASan configuration because opening a real GPU window makes
Mesa's LeakSanitizer report a 256-byte driver-side leak. That is expected; do not report it as a
project leak. `release_check` may report SKIP for tools that are unavailable; a SKIP is not proof.
Display/GPU-dependent integrations (`sdf-device`, `sdf-state-probe`, `sdf-display-skip`) classify
an unusable session as SKIP (exit 77) by actually probing `SDL_Init`/`SDL_CreateGPUDevice`; do not
reintroduce environment-variable heuristics such as "DISPLAY is set", because tool sandboxes can
inherit dead display variables. `sdf-display-skip` forces a missing SDL video driver to verify the
77/1 classification deterministically.

### Windows verification (experimental)

Windows is verified on the operator-provided MSYS2 UCRT64 machine; never commit credentials or
tokens. Keep the tree in sync (network, or a `git bundle` when the remote is unreachable).

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
cd D:\CLang\NandinaUI
git pull --ff-only            # or git fetch <bundle> refs/heads/main:refs/remotes/bundle/main
meson subprojects download    # SDK packaging fixtures bundle every wrap source (incl. optional box2d)
meson setup --reconfigure buildDir
ninja -C buildDir
meson test -C buildDir --print-errorlogs   # full suite: unit + integration (latest record 129/129 as of 2026-11-13)
```

- A reused build directory may keep a stale thin archive after a target-set change; delete
  `buildDir\nandina\libnandina.a` (or configure a clean directory) and rebuild.
- SSH runs in a non-interactive session: GPU/swapchain-dependent checks fail with
  `0x887A0022`. Run GPU device tests, SDF goldens, and any GUI interaction in an interactive
  desktop session; CPU paths can use `--renderer sdl2d` over SSH.
- The CPU-only tests (`nanres-cli`, `nanres-build-workflow`, `portable-stage`, `nanres-keys`,
  `notebook-data`) do run over SSH and belong to Windows verification: they exercise `pack`,
  `unchanged`, external files, portable staging, the generated key header with a real C++ compile
  and the notebook data layer (resource-driven migrations + repositories). They found both Windows
  packaging defects fixed in `3c86927`, so do not treat `--suite unit` alone as proof that
  packaging works.
- As of 2026-11-09 the **full** Windows suite is green (125 OK / 0 FAIL) after fixing five tooling
  portability defects: shader-artifact CRLF hashing, quoted/backslash `compile_commands` parsing in
  the header gate, POSIX fake paths in the format gate, `.obj` object suffixes in the tool-dependency
  fixture, and byte-exact fixture checkouts via `.gitattributes` (`-text`). Do not reintroduce a
  raw-byte assumption on a text file without that attribute. The SDK consumer fixture
  (`sdk-vendored-consumer`) is registered on Linux only: its `ldd`/rpath assertions are Linux
  loader semantics, and a deep Windows temp build path hits the 32K `CreateProcess` command-line
  limit. Windows SDK consumer delivery stays unverified. Record:
  `docs/references/windows_acceptance.md` §9.
- **High-DPI content scaling (P4.5a, 2026-11-09) and mixed-DPI multi-monitor (P4.5b, 2026-11-10)
  are implemented and verified on this machine** (primary 2880×1800@175%, second 2560×1440@125%).
  `WindowConfig::width/height` and the whole scene/input/IME seam are logical units; the SDL
  platform layer sizes the native window by `content_scale = display_scale / pixel_density`
  (1.75 / 1.25 here), sets the renderer scale to `display_scale`, converts pointer coordinates
  back to logical space and scales `SDL_SetTextInputArea`; text rasterizes at physical density;
  scale changes recompute all of the above, restore the author's logical window size and clamp
  the frame into the target display's usable area. Verified: window opens at 2100×1400 physical
  (175%); moving to the 125% display keeps logical 1200×800 with native 1500×1000 and
  render_scale 1.25; a real title-bar drag across the boundary works; a resize on the 125%
  display carries back as 2240×1540 at 175%; physical-coordinate sidebar clicks switch pages at
  both scales. Diagnostic log line per scale event: `SdlPlatformWindow: display scale …
  logical=… native=… pixels=… render_scale=…`. Tests/probes/goldens must use
  `WindowConfig::high_dpi=false` (fixed 1×) — the page probe has its own 1× host, so existing
  goldens are unaffected; do not compare goldens across DPI. Contract, evidence and residual
  mixed-DPI items: `docs/references/high_dpi_and_multi_display.md`. Real IME candidate placement,
  Linux Wayland fractional scaling remain unverified; do not claim full high-DPI support.
- **Manual visual acceptance (2026-11-10) passed for the showcase**: the maintainer reviewed
  10 page captures (5 pages × light/dark) plus 6 interaction states at 175% (evidence:
  `docs/images/windows_visual_2026-11-10/`, record in `windows_acceptance.md` §8). Two findings:
  the Label page "Font style" specimens look identical because the builtin default family ships a
  single face (Caskaydia Cove Regular) and there is no synthetic bold/italic — **fixed by P4.7 on
  2026-11-10** (graded synthetic bold + oblique, page goldens unchanged; see
  `docs/references/font_synthesis.md`); and editable `TextField`s had no hover feedback — the
  default recipe now paints an accent hover fill plus a ring border (focused resets both), covered
  by `[widget][text-field][hover]` and `[theme][text-field][hover]` tests. Re-check the visual
  evidence before changing that rule.
- Windows refuses to rename a directory while a file inside it is open. Finalize prepared
  statements and close the SQLite connection with `sqlite3_close` (a `sqlite3_close_v2` with live
  statements defers the close and keeps the handle) before renaming the package directory.
- Keep format files byte-stable across platforms: write them with binary streams when a text-mode
  CRLF would change the on-disk format, and tolerate a trailing `\r` when reading legacy files
  (the package fingerprint is the current example).
- Sync without network: on Linux create a bundle with `git bundle create <file> main --not <old>`,
  copy it over, fetch it to `refs/remotes/bundle/main`, and `git reset --hard` that ref.
- The Windows machine currently has MSYS2 clang-format 23.1.3, which the format gate rejects by
  design. For `release_check`, install the pinned tool into a venv and put it first on `PATH`:
  `python -m venv buildDir\format-venv; buildDir\format-venv\Scripts\pip install clang-format==23.1.1`.

## Code Conventions & Common Patterns

- **Namespaces**: one module namespace `nandina::<module>` per directory; sub-namespaces such as
  `widget::authoring`, `widget::primitives`, `foundation::utf8`. Import individual names; never
  `using namespace`.
- **Naming**: public framework types use the `Nan` prefix; files `snake_case`; functions/methods
  lower-case; private members trailing `_`; `enum class` values lower-case; macros upper-case.
- **Signatures**: trailing return types, `[[nodiscard]]` on queries, `noexcept` on non-throwing
  handles/destructors, concepts instead of runtime checks. Header guards are
  `NANDINA_EXPERIMENT_<MODULE>_<FILE>_HPP` (historical literal).
- **No RTTI**: builds may use `-Dcpp_rtti=false`. No `dynamic_cast`/`typeid`. Use explicit virtual
  `as_*()` hooks or `static const int` type-token addresses (`page_key`, `params_key`, `store_key`).
- **Errors**: recoverable failures use `std::expected` with typed structs (`ResourceError`,
  `FontError`, `RoutesError`, `NavigationError`). Exceptions are for misuse/call-order only.
  `navigate()` reports failures through `NanWindow::on_error`, not exceptions.
- **Threading**: the reactive graph is single-threaded. Background work uses
  `BackgroundExecutor`/`AsyncScope` and re-enters via `UiDispatcher::post`/`drain` in the `tasks`
  phase. Do not call `Signal::set` from a worker. Caches take their own locks; texture uploads
  happen on the UI thread.
- **DI seams**: `IPlatformWindow`, `IRenderDevice`, `IImageDecoder`, `IResourceBackend`,
  `ITextLayoutBackend`/`ITextLayoutRenderer`, `FontLoader`, `ThemeManager`, `TextureCache`,
  `IClipboard`. Inject through application/window/build context; no service locator/globals.
- **State**: `Signal<T>`, `Computed<T>`, `Effect`/`EffectScope`, `Event`/`Subscription`,
  `Property<T>`, `batch(...)`; lifetime and callback guarding belong to `ReactiveScope`
  (invalidate generation → events → effects → computeds → signals).
- **Resources**: carry full `ResolvedResource`/`ResolvedStream` scope through consumers. Never key
  a shared cache by raw ID/key alone; use `scope_identity(...)`. Honor
  `ResourceConsistency` and `invalidate(resolved)` vs `invalidate_wide(...)`.
- **Resource identity**: `[[resources]]` in `resources.toml` may declare `key` together with a
  relative `path`; an explicit mapping wins over the path-derived key, the file may live outside
  the scan roots, and directories, absolute/rooted paths or `.`/`..` are rejected. The scanner
  receives those paths through `ResourceScanOptions::mapped_paths` and neither derives a key nor
  reports diagnostics for them, so a root file whose path cannot form a canonical key can still be
  mapped explicitly. Compare path identity with `resource_path_identity(...)` (absolute + normalized
  generic string, ASCII case-folded on Windows); never compare raw `generic_string()` spellings.
  Derived keys follow `derive_resource_key(prefix, relative)` = `prefix + "/" + relative`;
  `ResourceKey` still only accepts lowercase canonical segments. New record IDs come from
  `derive_resource_id(package_id, key)` (version prefix + NUL + big-endian u32 lengths +
  package_id + key, SHA-256 truncated to 128 bit); it is implemented in
  `tools/resource_manifest.cpp` (tools layer, OpenSSL) so keep it out of the runtime core. Never
  invent random IDs for new keys. `build_resource_lock` always writes derived IDs; `scan` fails on
  legacy random IDs unless `--rewrite-lock` is explicit, and lock loading lists duplicate-ID or
  duplicate-key collisions. `--lock`/`--rewrite-lock` are manifest-mode only (raw scan overrides
  reject them); a lock whose `package_id` differs from the policy fails before migration advice.
  The build helper writes the lock to `<output>.lock.toml` in the build tree (`--lock <path>`); do
  not reintroduce content-hash identity inheritance or source-tree lock writes from the normal
  build. `nanres keys --header <path>` emits the optional author constant header: logical keys
  only, `RESOURCE_*` symbols inside `nandina::resource_keys::pkg_*` (package-id `.`/`-`/`_` are
  escaped as `DOT`/`DASH`/`UNDERSCORE`), alias constants included, symbol collisions fail loudly,
  and an unchanged header is not rewritten. Treat it as a build-side generated input, never as a
  hand-maintained source of truth.
- **Components**: follow `docs/references/adding_a_component.md`; recipe/default/style edits span
  `theme/visual_state.hpp`, `design_system.hpp/.cpp`, and component code. Use semantic tokens.
- **Formatting**: clang-format 23.1.1 only, column limit 100, 4-space indent, namespace
  indentation, pointer-left. Format changed files only; never reformat the repo.
- **New files**: add `.cpp/.hpp` to the relevant list in `nandina/meson.build` (no globbing);
  classify new headers in `tools/header_surface.toml` and confirm their level manually.

## Important Files

- `nandina/app/nan_application.hpp`, `nandina/app/nan_window.cpp` (frame loop, `tick()` ~514-640),
  `nandina/app/nan_router.hpp`, `nandina/app/nan_page.hpp`
- `nandina/foundation/input.hpp` (key identities), `nandina/scene/input_event.hpp`
- `nandina/reactive/graph.hpp`, `signal.hpp`, `computed.hpp`, `effect.hpp`, `scope.hpp`, `property.hpp`
- `nandina/scene/scene_tree.hpp`, `control.hpp`, `node.hpp`, `frame_scheduler.hpp`
- `nandina/render/render_device.hpp`, `draw_context.hpp`, `texture_cache.hpp`, `image_decoder.hpp`
- `nandina/resource/resource_manager.hpp`, `resource_backend.hpp`, `resource_stream.hpp`
- `nandina/text/text_layout.hpp`, `font_loader.hpp`, `font_pipeline.hpp`, `glyph_atlas.hpp`
- `nandina/theme/design_system.hpp`, `theme_manager.hpp`
- `nandina/widget/authoring.hpp`, `build_context.hpp`, `controls.hpp`
- `nandina/app/platform_window.hpp` (host seam), `nandina/render/recording_device.hpp` (internal probe)
- `meson.build`, `meson_options.txt`, `nandina/meson.build`, `nandina/resource/meson.build`,
  `tests/meson.build`, `tests/resource/meson.build`
- `tools/header_surface.toml`, `tools/check_format.py`, `tools/check_headers.py`,
  `tools/check_test_suites.py`, `tools/release_check.py`, `tools/check_tool_dependencies.py`,
  `tools/capture_backend_golden.py`, `tools/compare_backend_golden.py`,
  `tools/approve_backend_golden.py`, `tools/check_backend_budget.py`, `tools/copy_runtime_dlls.py`
- `docs/references/development_loop.md`, `coding_conventions.md`, `module_dependency.md`,
  `sdk_surface.md`, `dependency_policy.md`, `adding_a_component.md`

## Runtime/Tooling Preferences

- **OS/compiler**: Linux desktop; GCC ≥ 16 or Clang ≥ 21 with C++26. Ninja, CMake, pkg-config,
  Python 3.11+ (`tomllib`); system OpenSSL ≥ 3.0 when building resource tools.
- **Meson**: CI pins **1.7.2**. Reconfigure only after build-file changes; build dirs are ignored
  and local names include `buildDir`, `buildDir-asan`, `buildDir-nosqlite`, `buildDir-no-rtti`,
  `buildDir-tools`.
- **Dependencies**: default `vendored`; wraps are `wrap-git` with full 40-hex commit pins.
  SDL3 supports `system|vendored|auto` and static/shared. SQLite is the in-tree amalgamation under
  `third_party/sqlite3`; only the font data is a git submodule. Use
  `meson subprojects download` then `--wrap-mode=nodownload` for offline builds.
- **Tooling**: clang-format is pinned to **23.1.1**; `check_format.py` refuses other versions.
  `.clang-tidy` is editor/optional and not run by CI. `.clangd` expects `buildDir/compile_commands.json`.
- **Platforms**: Windows uses MSYS2 UCRT64 (experimental); macOS is unsupported. Do not claim
  Windows/macOS verification without real evidence.

## Testing & QA

- **Frameworks**: Catch2 v3 for C++; Python 3 scripts for CLI/build/workflow tests. No pytest.
- **Registration**: every test must declare suite `unit` (C++) or `integration` (Python/build).
  Register in `tests/meson.build` or `tests/resource/meson.build`; missing suites silently lose
  sanitizer coverage and fail `tools/check_test_suites.py`.
- **Red first**: a new capability needs a test that fails when the relevant implementation line is
  deliberately broken. Record the failing check, then make it pass. Add regression tests for
  bugs; avoid tests that merely re-pin incidental wording/implementation.
- **Determinism**: fixed fonts/sizes/frame counts, analytic expected values, tolerances only where
  necessary. Do not assert whole-image means or duplicate existing coverage.
- **Suites**: normal full suite, ASan/UBSan unit, no-RTTI unit, no-SQLite unit, and the pure-tools
  build are the standard matrix; compile and test in one `&&` chain. The ASan configuration keeps
  window tests disabled (driver-side LSan); `LSAN_OPTIONS=detect_leaks=0` is available when a local
  font-pipeline allocation report needs to be separated from a real failure.
- **Skips**: window tests map Catch2 skip to Meson 77; `window-lifecycle` is skipped in ASan on
  driver-side LeakSanitizer. GPU/SDF page-golden and performance work are local/release checks;
  the CPU-path page-golden gate is a **blocking** CI workflow (`visual-golden.yml`, promoted
  2026-11-14) that auto-covers every approved Linux CPU page; do not add new pages to the
  workflow's page list — approve their manifest entry instead.
- **Golden/visual**: page goldens compare per-channel tolerances against approved entries; capture
  never overwrites approved baselines, and approval needs human review metadata. Never copy
  baselines across platforms. The plain-language overview and the full capture/approve workflow
  live in `docs/references/visual_regression.md` §0; a new page only gets a baseline after it is
  registered in `probe/cross_platform/desktop/page_probe.cpp` (with a mounted resource package
  when it uses `res://`), captured per platform/render path, reviewed and approved.
- **Coverage expectation**: no numeric percentage gate; require behavioral coverage of new public
  API/boundaries and configuration-specific builds. Do not edit tracked files during a full suite
  run, and do not trust a green `release_check` that reports SKIPs for unavailable tools.
- **Example app test placement**: deterministic data-layer tests stay in the repository
  (`examples/notebook/tests/`, registered as `notebook-data` on the `unit` suite so CI and Windows
  cover them); UI/interaction and long-running app tests live in the untracked `playground/` and
  are never cited as CI evidence. Reusable widgets are authored in `showcase/` with its visual
  acceptance, then composed by the notebook.

### D2 review history

D2 step-3/4/4a–4d/Shell review cycles are closed (commits `995e6ae`, `4746173`, `fef829a`,
`3e2542a`, `f383201`, `5061b39`, `f821f40`, `d9d234c`). Per-step evidence, Windows readback and
the remaining color-gate boundaries live in
[theme_authoring_and_state.md](docs/references/theme_authoring_and_state.md); the registration
contrast gate is not wired — do not claim the full color gate passes.
