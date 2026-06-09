cmake_minimum_required(VERSION 4.0)

nandina_add_test(
    NAME    test_render_scene
    SOURCES test_render_scene.cpp
    DEPS
        nandina::render
)
