# stb 单头文件库

`stb_image.h`（v2.30）与 `stb_image_resize2.h`（v2.18），来自
<https://github.com/nothings/stb>，Public Domain（或 MIT，见各自文件头）。

## 为什么放这里

SDF/渲染后端的图片解码需要一个**后端无关的 CPU 解码器**。此前解码实现依赖 raylib
（`LoadImageFromMemory`/`ImageCrop`/`ImageResize`/`ImageColorTint`）；raylib 退役后，
这里提供同样的 PNG/JPEG/BMP/GIF/TGA 解码与线性重采样能力，不把整个图形库带进依赖闭包。

## 版本与升级

- 两个文件是从 raylib 子项目自带的副本原样复制的（raylib 也基于同一份 stb）；
- 升级时替换文件并更新本 README 里的版本号，跑 `tests/image_decoder_tests.cpp`；
- 编译开关在 `nandina/render/image_decoder.cpp`（只启用所需格式、禁用 stdio）。
