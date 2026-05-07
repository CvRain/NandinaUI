cmake_minimum_required(VERSION 4.0)
project(nandina_showcase_tests LANGUAGES CXX)

nandina_add_test(
    NAME    test_showcase_layout
    SOURCES test_showcase_layout.cpp
    MODULES
        ${CMAKE_SOURCE_DIR}/showcase/pages/dashboard_page.cppm
        ${CMAKE_SOURCE_DIR}/showcase/components/sections/bottom_content_section.cppm
        ${CMAKE_SOURCE_DIR}/showcase/components/cards/chart_card.cppm
        ${CMAKE_SOURCE_DIR}/showcase/components/sections/footer_section.cppm
        ${CMAKE_SOURCE_DIR}/showcase/components/sections/header_bar.cppm
        ${CMAKE_SOURCE_DIR}/showcase/components/sections/dock_bar.cppm
        ${CMAKE_SOURCE_DIR}/showcase/components/sections/middle_content_section.cppm
        ${CMAKE_SOURCE_DIR}/showcase/components/cards/project_progress_card.cppm
        ${CMAKE_SOURCE_DIR}/showcase/components/cards/recent_activity_card.cppm
        ${CMAKE_SOURCE_DIR}/showcase/components/sections/sidebar_section.cppm
        ${CMAKE_SOURCE_DIR}/showcase/components/cards/stack_demo_card.cppm
        ${CMAKE_SOURCE_DIR}/showcase/components/sections/stats_section.cppm
    DEPS
        nandina::app
        nandina::layout
        nandina::widgets
)