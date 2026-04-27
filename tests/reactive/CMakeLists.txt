cmake_minimum_required(VERSION 4.0)

# ── Issue 018–028: State / Effect / Prop / StateList / batch / EventSignal ──
nandina_add_test(
    NAME test_reactive_core
    SOURCES test_reactive.cpp
    DEPS nandina::reactive
)