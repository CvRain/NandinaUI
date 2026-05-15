cmake_minimum_required(VERSION 4.0)
project(nandina_showcase_tests LANGUAGES CXX)

nandina_add_test(
    NAME    test_showcase_layout
    SOURCES test_showcase_layout.cpp
    MODULES
        ${CMAKE_SOURCE_DIR}/showcase/nan_showcase.cppm
        ${CMAKE_SOURCE_DIR}/showcase/showcase_registry.cppm
        ${CMAKE_SOURCE_DIR}/showcase/pages/dashboard_page.cppm
        ${CMAKE_SOURCE_DIR}/showcase/pages/layout_page.cppm
        ${CMAKE_SOURCE_DIR}/showcase/pages/widgets_page.cppm
        ${CMAKE_SOURCE_DIR}/showcase/pages/authoring_page.cppm
    DEPS
        nandina::app
        nandina::layout
        nandina::widgets
        nandina::text
)