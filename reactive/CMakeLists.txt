cmake_minimum_required(VERSION 4.0)
project(nandina_reactive LANGUAGES CXX)

# Reactive — 响应式核心
# 职责：State / ReadState / Computed / Effect / EffectScope / Prop / StateList / EventSignal
#        等响应式原语，依赖追踪上下文与批处理调度。
# 依赖：nandina::foundation
# 实现阶段：Issue 018–028（Milestone M2）

add_library(nandina_reactive INTERFACE)
add_library(nandina::reactive ALIAS nandina_reactive)

target_compile_features(nandina_reactive INTERFACE cxx_std_26)

target_link_libraries(nandina_reactive
    INTERFACE
        nandina::foundation
)

# 源文件将在实现阶段逐步添加，预期结构：
#
# target_sources(nandina_reactive
#     PUBLIC
#         FILE_SET CXX_MODULES FILES
#             src/state.cppm          # State<T> / ReadState<T>
#             src/computed.cppm       # Computed<R>
#             src/effect.cppm         # Effect / EffectScope
#             src/prop.cppm           # Prop<T>
#             src/state_list.cppm     # StateList<T>
#             src/event_signal.cppm   # EventSignal<Args...> / Connection
#             src/batch.cppm          # batch(fn) 批处理调度
#             src/tracking.cppm       # thread-local 依赖追踪上下文
# )
