cmake_minimum_required(VERSION 4.0)
project(nandina_showcase_tests LANGUAGES CXX)

nandina_add_test(
    NAME    test_showcase_layout
    SOURCES test_showcase_layout.cpp
    MODULES
        ${CMAKE_SOURCE_DIR}/showcase/chart_card.cppm
        ${CMAKE_SOURCE_DIR}/showcase/header_bar.cppm
        ${CMAKE_SOURCE_DIR}/showcase/dock_bar.cppm
        ${CMAKE_SOURCE_DIR}/showcase/recent_activity_card.cppm
        ${CMAKE_SOURCE_DIR}/showcase/stats_section.cppm
    DEPS
        nandina::layout
        nandina::widgets
)