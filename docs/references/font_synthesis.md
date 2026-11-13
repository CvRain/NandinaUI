# 合成字重/斜体回退（P4.7）

> **状态（2026-11-10）**：已实施。内置默认字体族只有 Caskaydia Cove Regular 一个 face，
> `medium/bold/italic` 请求过去按“最近 face”解析到同一字面；现在在字体管线中做**合成**：
> 缺对应 face 时用 FreeType 加粗/倾斜生成字形，并用 HarfBuzz synthetic bold 同步推进宽度。
> 不改默认字体资产与许可，真实多 face 资源仍是后续增强项。

## 1. 动机

媒体证据（`docs/images/windows_visual_2026-11-10/`，`windows_acceptance.md` §8）显示
Label 页 Font style 四种标本视觉相同；根因是默认族只注册 Regular face，且管线没有合成回退。
P4.6 的 role/字重体系（heading/display weight 600 等）视觉验收也依赖字重可见，因此先实现
合成回退，避免 Step 2b 迁移时再单独重批一次 golden。

## 2. 决策规则

输入是 `theme::FontRequest`（weight 1–1000、slant）与解析到的 `FontFaceSpec`（真实 face 的
weight/slant）：

| 请求 | face | 结果 |
| --- | --- | --- |
| 400 | 400 | 不合成 |
| 500 | 400 | 合成加粗，强度约 0.014 em |
| 600 | 400 | 合成加粗，强度约 0.028 em |
| 700+ | 400 | 合成加粗，强度上限 0.0417 em（0x0AAA） |
| 500/600/700 | 同重量或更重 | 不合成（不支持变细） |
| italic/oblique | normal | 合成倾斜（剪切 ≈0.2126，约 12°） |
| 任意 | 已有 italic face | 不合成 |

强度按 `synthetic_bold_em × (request.weight − face.weight) / 300` 计算并夹在
`[0.012, 0.0417] em`，让 Medium/Semibold/Bold 三档有层次，而不是统一一个粗体。
`FontSynthesis { bold, oblique, bold_em }` 是该决策的纯值结果。

## 3. 管线接线

- `text/font_synthesis.hpp`：`FontSynthesis`、`synthetic_font_style()` 与常量；纯逻辑、可单测。
- `FreeTypeFontFace::rasterize/rasterize_glyph`：加载轮廓后按 `bold_em` 调用
  `FT_GlyphSlot_AdjustWeight`，按 slant 调用 `FT_GlyphSlot_Oblique`，再 `FT_Render_Glyph`。
- `GlyphAtlas`：构造时接收该图集的 `FontSynthesis`；图集与管线一一对应，缓存键不必变化
  （`FontPipelineCache` 已按 family/weight/slant/options 分桶，regular 与 bold 各自拥有图集）。
- `HarfBuzzTextLayoutBackend`：每个 hb_font 按同一 `bold_em` 调用
  `hb_font_set_synthetic_bold(..., in_place=false)`，让 shaping 的 advance 与加粗后的
  位图宽度同步；斜体只影响栅格，不影响 advance（与 FreeType 一致）。
- `FontPipeline`：从 `FontRequest` 与每个 face 的 spec 计算 synthesis 向量；fallback face
  也参与，保证混合字体时风格一致。

## 4. 边界与已知限制

- 只支持“变粗/倾斜”，不支持把更重的 face 变细；请求 800 而只有 700 face 时不会增强。
- 合成质量不等同真实 Medium/Bold/Italic face；CJK 字形同样适用，但笔画细节取决于字体轮廓。
- 倾斜固定约 12°；没有“真实 italic 存在时不用合成”之外的字体特性协商（如 variable font 的
  weight 轴仍走真实 face 解析）。
- 非轮廓字形（bitmap/color）无法斜切；加粗对 bitmap 面的行为由 FreeType 决定，未单独验收。
- 不做 synthetic small-caps 等其他合成。
- **2026-11-12 决策：本轮不投资真实多 face 资产**（许可、包体积、CJK 覆盖与全部页面 golden
  重批成本），合成回退继续作为默认路径。B1 收口后默认组件配方已在生产路径上消费 500/600
  字重，因此合成加粗成为默认可见视觉（按钮标签为 500）；若后续引入真实 Medium/Bold/Italic，
  需单独立批并重批页面 golden。

## 5. 视觉与 golden

- 默认视口 1200×800 的 Label 页 golden 只覆盖到 Font style 的 **Regular** 行，Regular 无合成，
  因此 P4.7 当时 6 条既有页面 golden **不变**（2026-11-11 本地化重批后每平台 8 条）（Windows 采集 `changed_pixels=0, max_channel_diff=0`；
  Linux 同批复核）。无需为 P4.7 重批基线。
- 为人工视觉验收，`page_probe` 新增 `NANDINA_PROBE_WIDTH/HEIGHT`（仅手动采集；golden 命令
  仍用默认 1200×800）。渲染 1200×1400 可见 Medium/Bold/Italic 三行差异。
- Step 2b 的 role 迁移会让组件真正消费 600/500 字重：**2026-11-12 B1 已在默认配方上完成**
  （按钮 label role = 500），页面 golden 按正常流程重采并由维护者批准；label/typography 页
  0 像素差异、home/anchors 差异集中在按钮文字，符合预期。

## 6. 验证（2026-11-10）

- 单测 `[text][synthesis]`：决策表（400/500/600/700、已有 medium/bold/italic face、变细拒绝）、
  分级强度（Medium < Semibold < Bold 上限）；FreeType 栅格加粗的 advance/bitmap 变化与倾斜
  的不变 advance；HarfBuzz 加粗后 shaping 宽度增大；GlyphAtlas 带 synthesis 时缓存字形更宽。
- 红先：临时关闭 FreeType 合成后，`GlyphAtlas applies synthesis to cached glyphs` 失败
  （`16.0f > 16.0f`）；恢复后通过。
- 双平台全量套件与 release gates 在提交时运行；P4.7 当时页面 golden 6 条保持 0 差异（现每平台 8 条）。
