cmake_minimum_required(VERSION 4.0)
project(nandina_showcase LANGUAGES CXX)

# Showcase — 展示与验证应用
# 职责：以可运行应用验证组件、布局与主题能力，不作为框架核心依赖。
# 依赖：nandina::app（待实现后解注释）

set(PROJECT_CPP_FILES
        main.cpp
)

set(PROJECT_CPP_MODULE_FILES
        pages/dashboard_page.cppm
        components/sections/bottom_content_section.cppm
        components/cards/chart_card.cppm
        components/sections/dock_bar.cppm
        components/sections/footer_section.cppm
        components/sections/header_bar.cppm
        components/sections/middle_content_section.cppm
        components/cards/project_progress_card.cppm
        components/cards/recent_activity_card.cppm
        components/sections/sidebar_section.cppm
        components/sections/stats_section.cppm
        components/cards/stack_demo_card.cppm
)

add_executable(${PROJECT_NAME} ${PROJECT_CPP_FILES})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_26)

target_sources(${PROJECT_NAME}
        PUBLIC
        FILE_SET CXX_MODULES FILES ${PROJECT_CPP_MODULE_FILES}
)


target_link_libraries(${PROJECT_NAME}
        PRIVATE
        nandina::app
        nandina::layout
        nandina::widgets
)
