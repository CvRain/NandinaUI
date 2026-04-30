cmake_minimum_required(VERSION 4.0)
project(nandina_reactive LANGUAGES CXX)

# Reactive — 响应式核心
# 职责：State / ReadState / Computed / Effect / EffectScope / Prop / StateList / EventSignal
#        等响应式原语，依赖追踪上下文与批处理调度。
# 依赖：nandina::foundation
# 实现阶段：Issue 018–028（Milestone M2）

add_library(nandina_reactive)
add_library(nandina::reactive ALIAS nandina_reactive)

target_compile_features(nandina_reactive PUBLIC cxx_std_26)

target_link_libraries(nandina_reactive
    PUBLIC
        nandina::foundation
)

target_sources(nandina_reactive
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/nan_reactive.cppm       # 聚合模块 nandina.reactive
            src/tracking.cppm           # 依赖追踪上下文 / 批处理基础设施
            src/event_signal.cppm       # Connection / ScopedConnection / EventSignal<Args...>
            src/state.cppm              # State<T> / ReadState<T>
            src/computed.cppm           # Computed<F>
            src/effect.cppm             # Effect / EffectScope
            src/prop.cppm               # Prop<T>
            src/state_list.cppm         # StateList<T> / ListChange / ListChangeKind
            src/batch.cppm              # batch(fn)
            src/nan_animation.cppm      # Easing / NanAnimation / AnimationManager
)
