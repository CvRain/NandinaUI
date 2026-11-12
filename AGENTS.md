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

### Current milestone state (2026-11-03)

- P1 and P2 are closed. P2 covers the tool/UI split, startup policy, scoped manager results,
  cache identity/reclamation, resource identity (explicit `key → path`, frozen key derivation,
  versioned 128-bit IDs), lock migration (build-dir lock, explicit `--rewrite-lock`, collision
  diagnostics, no-rewrite determinism), the minimal author constant header (`nanres keys`), and
  the P2.4 multi-route lab example (retired 2026-10-28; kept in the docs as historical evidence).
- The integrated example application is now `examples/notebook`: a personal notes app that grows
  batch by batch. Its SQLite data layer, account service (PBKDF2 hashing, registration, login)
  and login/registration/home routes are in place; the signed-in notebook workspace is still a
  placeholder. Components are authored in `showcase/`; UI tests live in the untracked `playground/`.
- P3 is in progress: **P3.1a** (per-event pointer coordinates/modifier snapshots through the
  host seam) and **P3.1b** (framework-owned `LogicalKey`/`PhysicalKey` identities, press/repeat/
  release lifecycle, SDL scancode/keycode mapping, unknown identity counting) and **P3.1c**
  (ordered cross-type `FrameInput::events` dispatched in capture order) are implemented, and
  **P3.2a** (D1 focus-hint core: `InputModality`/request context, `should_show_focus_hint()`,
  window activation gate, paint-only invalidation, widget ring migration) and **P3.2b**
  (showcase acceptance page, active-tab hint, popup enter/restore reasons, read-only field
  capability, always-visible switch) and **P3.3a** (text input session protocol:
  `ITextInputService`/`ITextInputTarget`, session identity + late-event filtering, single
  committed-text path, SDL composition events into the frame stream) are implemented, and
  **P3.3b1** adds inline preedit painting, password-purpose masking and value<->visual caret
  mapping. **P3.3b2a** adds caret mapping through the focus node, world and viewport into
  window logical coordinates, plus SDL input_area forwarding. **P3.3b2b** partially delivers
  SDL purpose/multiline properties and composition UI hints, with Linux Wayland IME acceptance.
  X11 inline preedit/standalone Xorg, Windows and candidate placement acceptance remain;
  desktop soft keyboard/visibility/occlusion are unsupported. See the roadmap.
- **P4.1a** (2026-11-03) adds Text/Label vertical alignment, constrained two-axis authoring
  entries, paint-only style/cache behavior, viewport alignment/clip fixes, and Label page /
  ComponentInfoTable resize regression coverage. Remaining D6 API/binding/measurement decisions
  stay in P4.1; this does not close the remaining P3 platform acceptance items.
- The **P4.1 text authoring/binding sub-batch** (2026-11-03) adds typography/overflow/
  max-lines builders, current layout queries, and one managed text binding shared by Property,
  Label and BuildContext. Direct writes (even equal values) replace text bindings; tree exit
  or page-scope teardown stops them. Other component bindings and style reset remain separate.
- **P4.1b** (2026-11-03, implemented and pushed in `0e7fca1`) adds a Surface
  layout/paint primitive via `ui.surface()`, explicit Card content insets with recipe reset,
  and percentage Image cover anchors. Surface/Card corners do not mask children: the existing
  scene clip is rectangular. Showcase visual acceptance remains a separate check.
- **P4.2 phase-5 first batch** (2026-11-04, maintainer review completed and pushed in
  `5d91e01`) establishes the data component basis: `widget::SelectionModel<Key>` (identity-based
  single/multiple selection, range + anchor, retain-on-source-replacement) and
  `widget::VirtualListView<Item, Key, NodeT>` (uniform-row virtualization via an internal
  ForEach window region, own vertical scroll viewport, click/keyboard selection). The review
  added source-generation pairing for same-window replacement, pre-layout pending scroll,
  bottom-edge hit clamping, unconditional select_range scroll, typeahead buffer resets,
  `ForEach::remove_and_delete` for traversal-safe window shrink, and by-value label callbacks
  with an internal snapshot cache (ASan-clean). Contract:
  `docs/references/data_components_and_virtualization.md`.
- **P4.2b static Table layout** (2026-11-05, maintainer and final code review completed)
  adds shared fixed/weighted column tracks, natural row heights, table/row/header/cell semantics,
  and typed Builder slots. Gap sums are validated before content changes; measured content
  extents are accumulated in double and rejected before publishing non-finite geometry.
  Showcase visual acceptance remains pending. It consumes ordinary controls and inherited text
  styles; selection, sorting and virtualization remain DataTable responsibilities. Contract:
  `docs/references/data_components_and_virtualization.md` §8.
- **P4.2c Collapsible** (2026-11-05, implemented and pushed in `095336b`) adds
  a retained disclosure content slot composed with Button, silent set_open/user toggle events,
  scoped Signal<bool> authoring, focus/capture restoration and optional semantics expanded.
  State merging and content replacement reentry have regression coverage. Contract:
  `docs/references/disclosure_components.md`.
- **P4.3 D3 navigation groups** (2026-11-05, maintainer review completed and pushed in
  `095336b`) renames `RouteOptions::type` to `nav_group` (default empty) and
  `NavSection::type` to `group`, without compatibility fields. Empty groups merge globally
  and omit headings; section dividers remain. Real Sidebar semantics/activation and router
  grouping have regressions. Desktop/GPU and visual acceptance remain pending. Contract:
  `docs/references/page_and_router.md`; evidence: `docs/references/reference_draft.md` D3.
- **P4.3 D2 theme presentation** (2026-11-05, reviewed and pushed in `8920f3a`)
  adds the §5.5 narrow read-only projection (`ThemePresentationState` = effective appearance +
  opaque revision) with a scope-bound `ThemePresentation` adapter (`ui.theme_state()`), observer
  lifetime safety in both destruction orders, equivalent-state deduplication, and a showcase
  appearance switch that follows external changes. Author color entry and the wider scheme/
  selection model remain separate. First-batch review was closed and pushed in `8920f3a`. Contract:
  `docs/references/theme_authoring_and_state.md`.
  The 2026-11-06 review fixes same-snapshot named appearance updates and active family replacement,
  stages internal observers/projections/effects/subscriptions, rejects reentrant writes before mutation,
  and completes necessary delivery before rethrowing callback failures. Notification state survives
  scope/manager destruction; readonly views remain borrowed. Pushed in `8920f3a`.
- **P4.3 D2 step 3 scheme catalog and selection** (2026-11-06, reviewed and pushed in
  `995e6ae`) adds strongly typed `ThemeSchemeId`/`ThemeFamilyId` (namespace:name text,
  typed parse errors, 255-byte cap), `ThemeSchemeContent`/`ThemeSchemeCatalog` (register/replace/
  remove with staged failure-atomic commits), `ThemeSelection` (fixed/appearance plus explicit
  `UnregisteredThemeSource`), removes legacy ThemeManager write entries, and compiles styles.toml
  themes/families into `styles:x<hex>` canonical IDs. Test compatibility layer (theme_test::Manager)
  keeps historical dual-palette queries working. Author color entry and step 4a color validation
  remain separate. The step-3 review cycle is closed. Contract:
  `docs/references/theme_authoring_and_state.md` §12–§13.
- **P4.3 D2 step 4 observation channels and author entry** (2026-11-07, reviewed and pushed in
  `4746173`) adds scope-bound read-only selection and catalog projections plus an explicit
  before/after theme change event, staged-only publication for selection/directory edits (no
  revision bump, no component invalidation), and the unified author entry (`derive_content`/
  `derive_scheme`/`validate_scheme` with structural diagnostics). File-import scope is documented
  in `design_tokens.md`. The step-4 review made equivalent catalog replacement idempotent, moved
  the directory-version check ahead of payload copying, and added channel lifetime, reentry,
  multi-projection, file-import and nested-derivation coverage. The step-4 review cycle is closed.
  Contract: `docs/references/theme_authoring_and_state.md` §14–§15.
- **P4.3 D2 step 4a CPU-side compositor and contrast diagnostics** (2026-11-07, reviewed and
  pushed in `fef829a`) adds the single foundation CPU compositor
  (`foundation/composition.hpp`: explicit encoded-sRGB strategy, background-stack early
  termination, composition errors) and scenario-scoped contrast rules/profiles
  (`theme/contrast_diagnostics.*`) with passed/failed/insufficient-context statuses, role paths,
  measured ratios, thresholds and resolved OKLCH plus converted sRGB. Local test helpers consume
  the shared compositor. Review fixes require explicit tone context for tone references, keep
  transform operand paths, stop sampling at the first opaque endpoint and report the composition
  config with clipping/float-no-u8 policy. Real pixel readback on the production software and GPU
  draw paths now compares framebuffer samples against the shared compositor (unit `sdl-visual`
  with ±2 tolerance, integration `sdf-device` with ±4, both including a wrong-space negative
  control). This validates encoded-sRGB composition on the Linux/Hyprland/Wayland + RADV offscreen
  target used by the test, and the same readback was re-verified on Windows 11 + D3D12 offscreen
  and default-swapchain targets (8-bit SDR; search for the SKIP lines when readback is unavailable)
  on 2026-11-09. The registration contrast gate is still not wired,
  Linux default-swapchain/other-driver cross-platform readback and the final gamut policy remain
  pending; do not claim the color gate passes. Contract:
  `docs/references/theme_authoring_and_state.md` §6.5–§6.8, §16.
- **P4.3 D2 step 4b author color entry** (2026-11-07, reviewed and pushed in `3e2542a`) adds
  typed primitive-palette access (`ReferenceColorFamily`, `reference_scale`, `reference_color`,
  family/shade names), explicit `NanColor` semantic overrides (`SemanticColorOverride`,
  `canonical_color_token`, `semantic_color`, `set_semantic_color`, atomic
  `apply_semantic_colors`) and the `ThemeSchemeContent::from_reference` /
  `from_semantic_colors` constructors with an optional `reference` provenance record copied by
  derivation. File palette overrides reuse atomic `apply_semantic_colors` after canonical-key
  precedence, so file import and C++ share validation, alias sync and commit semantics. No raw
  red/blue tokens are added and semantic tokens stay the default. Contract:
  `docs/references/theme_authoring_and_state.md` §17.
- **P4.3 D2 step 4c foundation named color literals** (2026-11-08, reviewed and pushed in
  `f383201`) adds `foundation/named_color.*`: `NanNamedColor` with
  the 148 CSS Color 4 named colors (pinned CSS Color 4 CRD 2026-11-03 §6.1 = SVG 1.1's 147 +
  rebeccapurple), typed `nan_named_color(NanNamedColor)`, case-insensitive
  `nan_named_color(std::string_view)` and `nan_named_color_name()`. The single constexpr table is
  private to the `.cpp`; aliases keep separate identities; enum ordinals are not stable (persist
  names). Query rejects whitespace, embedded NUL, non-ASCII and unknown names; no full CSS color
  syntax and no `transparent`/`currentColor`. Named colors are just explicit `NanColor` spellings,
  not semantic tokens; default recipes must not hardcode colors. TOML shorthand remains a
  separate batch. Review binds every expected color to its actual enumerator (ordinal-only tests
  missed swapped names), verifies uniqueness and RGBA bytes, all-name case folding and bounded
  string views. Source verification now matches all 147 entries in the maintainer-saved SVG 1.1
  document and all 148 in the fetched pinned CSS Color 4 snapshot; the former source-evidence gap
  is closed. Contract:
  `docs/references/theme_authoring_and_state.md` §18.
- **P4.3 D2 step 4d TOML named color literals** (2026-11-08, reviewed and pushed in `5061b39`)
  lets color-array fields accept a CSS named color string
  (`"SteelBlue"`) resolved through `nan_named_color()` with the same case/whitespace/error
  rules. `$palette.*` stays a semantic token reference and `$palettes.*` a reference-scale
  value; a failed reference match errors instead of falling back to a name, canonical palette
  names still win over aliases, errors carry the field path, and a failed import leaves the
  live catalog/selection/style untouched. Only named colors are added; other reference forms
  are not widened. Review preserves complete palette-validation input paths and recipe kind/zero-
  based index paths, covers every name and recipe color slot, and checks real parse/apply failure
  isolation after a successful import. Contract:
  `docs/references/theme_authoring_and_state.md` §19.
- **P4.3 D2 Shell multi-scheme framework preparation** (2026-11-08, reviewed and pushed in
  `f821f40`) replaces catalog errors' single scheme ID with
  operation-specific scheme/family subjects, adds scope-bound `on_theme_selection_change()` and
  `on_theme_catalog_change()` to `BuildContext`, and adds a shell action-sequence regression
  covering fixed/pair/system choices, inactive slot edits, replacements, dynamic catalogs,
  external synchronization, scope teardown and typed failure feedback. The showcase page body
  remains maintainer-owned. Contract: `docs/references/theme_authoring_and_state.md` §20.
- **P4.5a high-DPI content scaling** (2026-11-09, first increment of the maintainer-reported
  high-DPI defect) normalizes the platform seam to logical units: `WindowConfig` sizes are DIPs,
  `screen_size()`/pointer events/`TextInputArea` are logical, `drawable_size()` is physical,
  `display_scale()` is physical/logical, and the SDL platform layer creates the native window at
  `logical × content_scale`, sets the renderer scale, rasterizes text at physical density and
  handles display-scale change events. `WindowConfig::high_dpi=false` is the fixed 1× mode for
  tests/probes/goldens. `window_metrics` gained pure `content_scale_from_display` /
  `window_size_from_logical_size` / `logical_size_from_window_size` helpers with unit tests.
  Verified on this machine at 175% (1200×800 logical → 2100×1400 physical, crisp text, correct
  pointer hit) plus 1× regression. **P4.5b mixed-DPI multi-monitor is verified (2026-11-10)** on
  the real 2880×1800@175% + 2560×1440@125% pair: both-direction moves, a real title-bar drag
  across the DPI boundary, carrying a 125%-monitor resize back to 175%, logical-size stability
  with framebuffer/render-scale updates, physical-coordinate hit-testing at both scales, and
  usable-area clamping. Real IME candidate placement, Linux Wayland fractional scaling, display
  hot-plug/system-scale changes and accessibility text scaling remain pending; contract:
  `docs/references/high_dpi_and_multi_display.md`.
- **P4.6 Step 1 typography scale** (2026-11-10) adds a Tailwind-compatible
  `theme::TextSize` (13 steps: xs…9xl, spelled `xl_2…xl_9` in C++ because identifiers cannot contain `-`) and `theme::Leading` (6 multipliers) plus a themeable
  `NanTypeScale` (`root_base`, `ratios`, `leading`) under `NanTokens::typography.scale`;
  `text::TextStyle` gains a `line_height` multiplier (0 keeps the backend default; HarfBuzz and
  the deterministic backend both consume it) and `Label` gains
  `set_text_size()`/`set_leading()`/`set_line_height()` with builder methods
  (`text_size`/`leading`/`line_height`). Explicit `.font_size(px)` stays the absolute logical-px
  override and wins over the token (CSS px vs rem semantics); OS content scale always applies and
  no pt/physical-pixel font size exists. **P4.6 Step 2a** adds the semantic
  `TypographyRole` set (display/heading/title/subtitle/body/label/caption) with
  `Label::set_typography_role()` and builder `.typography_role()`. **Step 2b** makes the role table
  theme-overridable through `NanTypeScale::roles`, changes the unstyled Label default to `body`
  (16 px / normal 1.5; explicit size/text_size/inherited size keeps the backend line height), and
  adds the showcase Typography page (`content_and_display/typography`, also registered in
  page_probe) covering the 13-step scale, six leadings, seven roles and the body default. Default
  component recipes now consume semantic role **size** scalar tokens
  (`ScalarToken::typography_role_*`; styles.toml may reference `$tokens.typography.role.*`), so
  overriding `NanTypeScale::roles` changes component font sizes; role leading/weight/slant are
  not yet carried by `TypeStyle`, and a TOML role-table override is not open. **P4.6 Step 3**
  (2026-11-12) adds `NanTypeScale::text_scale` (rem-semantics user text scaling) and
  `max_text_px` (optional cap, 0 disabled): token/role sizes resolve as
  `root_base × ratio × text_scale`, then cap; explicit `.font_size(px)` stays fixed. OS
  accessibility scale source/runtime notifications remain pending. The 2026-11-11 Chinese
  re-batch approved 8 baselines per platform (home/anchors/label/typography × SDF/CPU).
  Contract: `docs/references/typography_scale.md`.
- **P4.7 synthetic weight/slant fallback** (2026-11-10) makes `FontRequest` weight/slant visible on
  single-face families: `text/font_synthesis.hpp` decides graded synthetic bold (500 ≈0.014em,
  600 ≈0.028em, 700+ ≤0.0417em) and ~12° oblique; `FreeTypeFontFace::rasterize*()` applies
  `FT_GlyphSlot_AdjustWeight`/`FT_GlyphSlot_Oblique`, `GlyphAtlas` carries the synthesis,
  `HarfBuzzTextLayoutBackend` sets `hb_font_set_synthetic_bold(..., in_place=false)` so shaped
  advances match the wider bitmaps, and `FontPipeline` computes per-face synthesis from
  `FontRequest` + `FontFaceSpec`. Real faces still win; thinning is unsupported; bitmap/color
  outlines are outside this batch. The default family/assets are unchanged; tests
  `[text][synthesis]` (decision/graded strength/raster/HarfBuzz/atlas) pass, red-first recorded,
  and page goldens remain 0-diff because the 1200×800 Label viewport only reaches the Regular
  specimen. Use `page_probe` `NANDINA_PROBE_WIDTH/HEIGHT` for tall manual visual captures.
  Contract: `docs/references/font_synthesis.md`.
- **P4.8 page load responsiveness (P4.8b implemented 2026-11-11)**: `PageContext::defer_section`
  registers a placeholder + builder; after the first frame the router builds at most one section
  per dispatcher drain and replaces the placeholder at its original sibling index (plain
  `replace_child()` appends, so the router removes+inserts). Pages can keep heavy text sections
  off the navigation transaction; retirement drops pending builders, builder errors go to the page
  error handler, and dispatcher-less hosts run all sections synchronously. The showcase Label page
  now builds only the intro synchronously and mounts Typography/Font style/colors/wrapping/
  overflow/alignment/reactive/semantics over the next 8 drains: first-frame page cost measured
  ~85 ms over anchors on Windows and ~38 ms on Linux (`FRAMES=1` probe deltas; was ~0.2–0.4 s
  blocking), full 1200×800 page golden remains 0-diff on both platforms.
  Tests `[app][router][deferred]` cover first frame, per-drain mounting, ordering, retirement,
  errors, the no-dispatcher fallback and page lifetime (red-first: disabling the pump failed 3
  cases with runs=0; restoring a strong root capture in the queued pump failed the lifetime test
  because the page tree outlived the render device on Linux `FRAMES=1/3` — the pump now captures
  the page root weakly). Remaining: optional timing log/bench, P4.8c build-time measurement
  batching, long-term async/incremental shaping. Contracts:
  `docs/references/page_and_router.md` §4.1 and `docs/references/page_load_responsiveness.md`.
- **High-DPI dropdown-menu regression (2026-11-11, fixed)**: opening a menu at 175% once shrank
  the whole app because `MenuItemNode`'s embedded `Text` members never received the scene-tree
  default text pipeline, so labels fell back to `SdlRenderDevice::draw_text()`'s debug font; that
  path reset `SDL_SetRenderScale` to 1.0 instead of restoring the platform's 1.75. `MenuItemNode`
  now forwards `apply_default_text_pipeline()`/`apply_font_context()` like Button, and
  `draw_text()` saves/restores the caller render scale. Regressions:
  `[dropdown-menu][render][text]` and `[visual][text][sdl][device]` (both red-first). At 175% the
  menu now opens without any page resize and its Chinese labels render through the real glyph
  pipeline. Contract: `docs/references/high_dpi_and_multi_display.md` §6.
- **GlyphAtlas narrow-growth fix (2026-11-11)**: `GlyphAtlas::grow_to_fit()` used to expand only
  the axis required by the current allocation. After a row wrap the requirement looks purely
  vertical, so the atlas grew from 256×256 into a 256×4096 strip while 94% of each row stayed
  unused; visiting showcase pages in order then made normal 25×22 glyphs fail with
  `does not fit a 4096x4096 atlas`, blanking Chinese text on the Input & Focus page. Growth now
  doubles both axes together (still capped at `kMaxDimension`); the 32×32-to-128 regression
  `[text][atlas][growth]` is red-first. Contract: `docs/references/text_shaping_cost.md` §11.
- **Showcase Chinese localization (2026-11-11)**: all user-facing showcase copy is now Simplified
  Chinese (component/API/token names stay English, page headings are bilingual like
  「标签（Label）」); multi-line descriptions use `theme::Leading::normal` (1.5) through
  `supporting_text` and explicit paragraph builders so Chinese body text has breathing room. The
  home page's previously empty 「组件速览」 heading now has quick-entry buttons that navigate by
  `PageKey`; probe route titles changed to 标签/排版 and the interaction fallback clicks 「标签」.
  The localized pages changed all page goldens; on 2026-11-11 the maintainer reviewed them and the
  dual-platform SDF+CPU baselines were re-captured and approved (8 entries per platform, 0-diff
  verification after approval).
- **Dual-platform verification (2026-11-10) is operational**: the Linux machine (Arch, GNOME
  Wayland, RADV RX 6750 GRE 12GB, Mesa 26.2.6) is reachable over SSH with a dedicated key; the
  full Linux suite is 129 OK / 0 FAIL and all eight Linux page goldens (home/anchors/label/
  typography × SDF/CPU) compare with 0 changed pixels. The 2026-11-12 re-check went through a new
  Network-address/port-forward entry and confirmed the same machine, repository path, key auth,
  129/129 suite and 0-diff goldens; the address is temporary and only kept in the local session.
  Workflow, session-environment exports (`XDG_RUNTIME_DIR`, `WAYLAND_DISPLAY`,
  `XDG_SESSION_TYPE=wayland`), remote shell caveat (default fish; run scripts via
  `/usr/bin/bash -s`), golden capture/approve rules and security boundaries are in
  `docs/references/dual_platform_verification.md`. Linux Clang 23 immediately caught a
  `logical_size_ = {}` portability issue (fixed in `d782729`) — Windows GCC success alone is not
  proof. Never commit remote credentials; run commits from Windows and `git pull` on Linux.
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
meson test -C buildDir --print-errorlogs   # full suite: unit + integration (125 OK as of 2026-11-09)
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

- `nandina/app/nan_application.hpp`, `nandina/app/nan_window.cpp` (frame loop, ~445-532),
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
  driver-side LeakSanitizer. GPU/page-golden and performance work are local/release checks, not CI.
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

### D2 step-3 review follow-up (2026-11-06, pushed in `995e6ae`)

StyleDocument stages every alias/family and commits catalog, selection and style once. Family member
IDs include `/families/x<family-hex>` so shared file themes do not change family ownership when
selection changes. Long aliases return diagnostics. Production manager regressions cover canonical
mutators, cross-family/fixed/anonymous selection, replacements, exceptions and lifetime; historical
test compatibility remains test-only. A fixed-source Sidebar switch restores the actual appearance.
The step-3 review cycle is closed.

### D2 step-4 and 4a status (2026-11-09)

The step-4 review cycle is closed and the batch is pushed in `4746173`: equivalent catalog
replacement is idempotent, staged publication and all channel lifetimes/exception paths are
covered, event endpoints own their snapshots and author derivation is deep. Step 4a's CPU side is pushed as `fef829a`: the single foundation compositor plus explicit
contrast rules/profiles/diagnostics, with local helpers migrated and the review fixes for tone
context, transform paths, opaque-endpoint sampling and policy reporting. Step 4b's author color entry
(typed primitive-palette access and explicit `NanColor` semantic overrides with file-import
parity) is pushed as `3e2542a`; its review closed non-finite file overrides through the shared
validator and covered every family/shade, role/alias, duplicate ordering and provenance. Step
4c's foundation named color literals (148 CSS Color 4 names, typed/string entries, private table)
are pushed as `f383201`; its review closed enum-identity test gaps and added exhaustive
byte/string-view boundaries, and source evidence matches both the SVG 1.1 and pinned CSS Color 4
snapshots. Step 4d's TOML named-color literals are pushed as `5061b39`, with complete error paths
and tests for all literals, reference boundaries and actual apply failure atomicity. Shell
multi-scheme framework preparation is pushed as `f821f40`, with typed catalog subjects,
scope-bound selection/catalog subscriptions and end-to-end interaction regressions. Environment
skip classification is pushed as `d9d234c`. The reviewed 4a follow-up adds software-renderer and
Linux/Hyprland+RADV production-path framebuffer readback against the shared encoded-sRGB
compositor, including wrong-space negative controls. **2026-11-09 Windows full verification**
(Windows 11 26200 + Radeon 660M/D3D12) adds the same software readback, a D3D12 offscreen
`composite readback matches the shared compositor (encoded sRGB, tolerance 4)` result and a
`default swapchain readback matches the shared compositor (encoded sRGB, tolerance 4)` result
(8-bit SDR target; SKIP + reason when readback is unavailable or the target is HDR), and the
Windows suite is fully green after five tooling portability fixes (see the Windows verification
section); the SDK consumer fixture is Linux-only. **P4.5a high-DPI content scaling is implemented
(2026-11-09)** — logical-unit window/input/IME seam, content-scaled native window (1200×800
logical → 2100×1400 physical at 175%), renderer scale and physical-density text raster, verified
on this 175% machine with crisp text and correct pointer hit; `WindowConfig::high_dpi=false` is
the fixed 1× mode for tests/probes/goldens. **P4.5b mixed-DPI multi-monitor is verified
(2026-11-10)** on the real 2880×1800@175% + 2560×1440@125% pair: both-direction moves, a real
title-bar drag across the DPI boundary, carrying a 125%-monitor resize back to 175%, logical-size
stability with framebuffer/render-scale updates, physical-coordinate hit-testing at both scales
and usable-area clamping. Real IME candidate placement, Linux Wayland fractional scaling,
display hot-plug/system-scale changes and accessibility text scaling remain pending; contract:
`docs/references/high_dpi_and_multi_display.md`. The registration
contrast gate, style advisories, the Shell page body/visual acceptance, Linux
default-swapchain/other-driver readback and internal broadcast narrowing remain pending; do not
claim the full color gate passes.
