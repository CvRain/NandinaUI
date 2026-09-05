//
// Created by cvrain on 2026/7/3.
//
// widget/reactive_control — 响应式控件基类 (缝合 scene + reactive)
//
// NanReactiveControl 继承 NanControl, 在其上挂一个 EffectScope, 让「状态变 → UI 变」
// 无需手写观察者样板:
//   - 子类覆写 bind(): 在其中用 effect 读 signal 并更新自己的节点属性
//     (set_position / set_background / set_visible ...);
//   - 生命周期钩子自动管理订阅: 节点 on_ready (挂入活动树, 子树就绪) 时调用 bind();
//     on_exit_tree (离开树 / 卸载) 时 clear() 断开全部 effect。
//
// 这契合 architecture.md 的分层: 内核 scene 保持纯净不依赖 reactive; 数据驱动的
// 胶水收敛在 widget 层。开发者写子类时只关心「哪个 signal 驱动哪个属性」,
// 不手工管理生命周期 / 作用域 / 挂载卸载 —— 对标 Qt/GTK/Slint。
//
// 依赖: scene (NanControl) + reactive (Graph/EffectScope)。
//

#ifndef NANDINA_EXPERIMENT_WIDGET_REACTIVE_CONTROL_HPP
#define NANDINA_EXPERIMENT_WIDGET_REACTIVE_CONTROL_HPP

#include "../reactive/effect.hpp"
#include "../reactive/graph.hpp"
#include "../scene/control.hpp"

namespace nandina::widget
{

    /// 带响应式订阅生命周期的控件基类。
    ///
    /// 用法: 继承它, 覆写 bind() 在其中注册 effect。effect 里读的 signal 变化时,
    /// effect 自动重跑更新本控件的属性。控件挂入树时 bind, 离开树时自动断订阅。
    class NanReactiveControl: public scene::NanControl {
    public:
        /// graph 的生命周期必须长于本控件 (通常由 app / 页面持有)。
        explicit NanReactiveControl(reactive::Graph& graph): scope_(graph) {}

        explicit NanReactiveControl(reactive::Graph& graph, foundation::NanSize size):
            scene::NanControl(size),
            scope_(graph) {}

    protected:
        /// 子类在此注册 effect。默认空实现 (纯静态控件)。
        /// 在节点就绪 (on_ready) 后调用; 此时节点已在活动树中, 可安全读取 tree/parent。
        virtual void bind(reactive::EffectScope& scope) {
            (void)scope;
        }

        /// 就绪时建立订阅。
        void on_ready() override {
            scene::NanControl::on_ready();
            bind(scope_);
        }

        /// 离开树 (卸载) 时断开全部订阅, 避免悬挂回调。
        void on_exit_tree() override {
            scope_.clear();
            scene::NanControl::on_exit_tree();
        }

        /// 供子类在 bind 之外 (如按钮回调) 追加一次性 effect。
        [[nodiscard]] auto effect_scope() -> reactive::EffectScope& {
            return scope_;
        }

    private:
        reactive::EffectScope scope_;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_REACTIVE_CONTROL_HPP
