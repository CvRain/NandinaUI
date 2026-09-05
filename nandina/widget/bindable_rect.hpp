//
// Created by cvrain on 2026/7/3.
//
// widget/bindable_rect — 一个最小的响应式控件示例
//
// BindableRect 是一个带背景色的矩形控件, 其 visible / background / position 可分别
// 绑定到一个 signal。绑定后, signal 变化会自动更新控件属性并反映到下一帧绘制,
// 无需手写回调 —— 这是 NanReactiveControl「状态驱动 UI」能力的最小可测演示。
//
// 绑定是可选的: 未绑定的属性保持静态值。绑定通过 attach_* 在挂入树前设置,
// 实际的 effect 在 on_ready 时由基类调用 bind() 建立。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_BINDABLE_RECT_HPP
#define NANDINA_EXPERIMENT_WIDGET_BINDABLE_RECT_HPP

#include "../reactive/signal.hpp"
#include "reactive_control.hpp"

namespace nandina::widget
{

    class BindableRect: public NanReactiveControl {
    public:
        BindableRect(reactive::Graph& graph, foundation::NanSize size):
            NanReactiveControl(graph, size) {}

        /// 绑定可见性到一个 bool signal。
        void bind_visible(reactive::Signal<bool>& source) {
            visible_src_ = &source;
        }

        /// 绑定背景色到一个 NanColor signal。
        void bind_background(reactive::Signal<foundation::NanColor>& source) {
            background_src_ = &source;
        }

        /// 绑定局部位置到一个 NanPoint signal。
        void bind_position(reactive::Signal<foundation::NanPoint>& source) {
            position_src_ = &source;
        }

    protected:
        void bind(reactive::EffectScope& scope) override {
            if (visible_src_ != nullptr) {
                scope.add([this] { set_visible(visible_src_->get()); });
            }
            if (background_src_ != nullptr) {
                scope.add([this] { set_background(background_src_->get()); });
            }
            if (position_src_ != nullptr) {
                scope.add([this] { set_position(position_src_->get()); });
            }
        }

    private:
        reactive::Signal<bool>* visible_src_ = nullptr;
        reactive::Signal<foundation::NanColor>* background_src_ = nullptr;
        reactive::Signal<foundation::NanPoint>* position_src_ = nullptr;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_BINDABLE_RECT_HPP
