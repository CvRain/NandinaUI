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

### Current milestone state (2026-10-24)

- P1 is closed. The P2 resource closure is implemented through **P2.3b**: tool/UI split, startup
  policy, scoped manager results, cache identity, cache reclamation, resource identity
  (explicit `key → path` mapping, frozen key derivation, versioned 128-bit derived IDs), and
  lock migration (build-dir lock, explicit `--rewrite-lock`, collision diagnostics, no-rewrite
  determinism).
- Next sub-batches: P2.3c (minimal constant header) and P2.4 (example application).
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

Useful options: `-Dbuild_ui=false` (resource tools only), `-Dresource_sqlite=disabled`,
`-Dresource_tools=false -Dexternal_nanres=/path/to/nanres`, `-Dsdl_backend=disabled
-Dshowcase=false` (headless), `-Dphysics2d=enabled`, `-Dcpp_rtti=false`.
`examples=true` is currently fatal (reserved). `resource_sqlite=auto` equals enabled.
`showcase` builds a resource package through the helper, so `-Dresource_tools=false` without
`-Dexternal_nanres` requires `-Dshowcase=false` (otherwise configure stops with an explanatory
error). Keep that in mind when configuring a tools-free sanitizer or minimal build directory.

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
5. **Commit one reviewable sub-batch per commit**, with a conventional prefix (`feat/fix/refactor/`
   `test/docs/perf/chore`) and a body that states behavior, verification numbers, and remaining
   boundaries. Push; retry transient network failures instead of leaving an unpushed commit.
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

### Windows verification (experimental)

Windows is verified on the operator-provided MSYS2 UCRT64 machine; never commit credentials or
tokens. Keep the tree in sync (network, or a `git bundle` when the remote is unreachable).

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
cd D:\playground\NandinaUI
git pull --ff-only            # or git fetch <bundle> refs/heads/main:refs/remotes/bundle/main
meson setup --reconfigure buildDir
ninja -C buildDir
meson test -C buildDir --suite unit --print-errorlogs
meson test -C buildDir nanres-cli nanres-build-workflow portable-stage --print-errorlogs
```

- A reused build directory may keep a stale thin archive after a target-set change; delete
  `buildDir\nandina\libnandina.a` (or configure a clean directory) and rebuild.
- SSH runs in a non-interactive session: GPU/swapchain-dependent checks fail with
  `0x887A0022`. Run GPU device tests, SDF goldens, and any GUI interaction in an interactive
  desktop session; CPU paths can use `--renderer sdl2d` over SSH.
- The CPU-only resource integration tests (`nanres-cli`, `nanres-build-workflow`, `portable-stage`)
  do run over SSH and belong to Windows verification: they exercise `pack`, `unchanged`, external
  files and portable staging. They found both Windows packaging defects fixed in `3c86927`, so do
  not treat `--suite unit` alone as proof that packaging works.
- Windows refuses to rename a directory while a file inside it is open. Finalize prepared
  statements and close the SQLite connection with `sqlite3_close` (a `sqlite3_close_v2` with live
  statements defers the close and keeps the handle) before renaming the package directory.
- Keep format files byte-stable across platforms: write them with binary streams when a text-mode
  CRLF would change the on-disk format, and tolerate a trailing `\r` when reading legacy files
  (the package fingerprint is the current example).
- Sync without network: on Linux create a bundle with `git bundle create <file> main --not <old>`,
  copy it over, fetch it to `refs/remotes/bundle/main`, and `git reset --hard` that ref.

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
  build.
- **Components**: follow `docs/references/adding_a_component.md`; recipe/default/style edits span
  `theme/visual_state.hpp`, `design_system.hpp/.cpp`, and component code. Use semantic tokens.
- **Formatting**: clang-format 23.1.1 only, column limit 100, 4-space indent, namespace
  indentation, pointer-left. Format changed files only; never reformat the repo.
- **New files**: add `.cpp/.hpp` to the relevant list in `nandina/meson.build` (no globbing);
  classify new headers in `tools/header_surface.toml` and confirm their level manually.

## Important Files

- `nandina/app/nan_application.hpp`, `nandina/app/nan_window.cpp` (frame loop, ~445-532),
  `nandina/app/nan_router.hpp`, `nandina/app/nan_page.hpp`
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
  baselines across platforms.
- **Coverage expectation**: no numeric percentage gate; require behavioral coverage of new public
  API/boundaries and configuration-specific builds. Do not edit tracked files during a full suite
  run, and do not trust a green `release_check` that reports SKIPs for unavailable tools.
