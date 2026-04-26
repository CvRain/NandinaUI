cmake_minimum_required(VERSION 4.0)

# ── Issue 011: 统一 Event 类型体系 ──────────────────────────────
nandina_add_test(
        NAME test_runtime_event
        SOURCES test_event.cpp
        DEPS nandina::runtime
)