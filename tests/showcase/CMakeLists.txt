cmake_minimum_required(VERSION 4.0)
project(nandina_showcase_tests LANGUAGES CXX)

nandina_add_test(
    NAME    test_showcase_layout
    SOURCES test_showcase_layout.cpp
    MODULES
        ${CMAKE_SOURCE_DIR}/showcase/nan_showcase.cppm
        ${CMAKE_SOURCE_DIR}/showcase/pages/sandbox_page.cppm
        ${CMAKE_SOURCE_DIR}/showcase/main_page.cppm
        ${CMAKE_SOURCE_DIR}/showcase/pages/button.cppm
        ${CMAKE_SOURCE_DIR}/showcase/pages/checkbox.cppm
        ${CMAKE_SOURCE_DIR}/showcase/pages/forms_page.cppm
    DEPS
        nandina::app
        nandina::layout
        nandina::widgets
        nandina::text
)