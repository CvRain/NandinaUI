//
// Created by cvrain on 2026/7/2.
//
// Factory for the raylib-backed render device. The concrete class and all raylib
// types stay inside raylib_device.cpp; callers only see IRenderDevice.
//

#ifndef NANDINA_EXPERIMENT_RAYLIB_DEVICE_HPP
#define NANDINA_EXPERIMENT_RAYLIB_DEVICE_HPP

#include "../render_device.hpp"

#include <memory>

namespace nandina::render
{

    /// Create a render device backed by raylib. Requires an active raylib window
    /// (InitWindow) before any draw calls. begin_frame/end_frame wrap
    /// BeginDrawing/EndDrawing.
    [[nodiscard]] auto make_raylib_device() -> std::unique_ptr<IRenderDevice>;

} // namespace nandina::render

#endif // NANDINA_EXPERIMENT_RAYLIB_DEVICE_HPP
