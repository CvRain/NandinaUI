cmake_minimum_required(VERSION 4.0)
project(nandina_showcase LANGUAGES CXX)

# Showcase — 展示与验证应用
# 职责：以可运行应用验证组件、布局与主题能力，不作为框架核心依赖。
# 依赖：nandina::app（含 Router / PageHost / Page 抽象）

set(PROJECT_CPP_FILES
        main.cpp
)

set(PROJECT_CPP_MODULE_FILES
        nan_showcase.cppm
        main_page.cppm
        pages/sandbox_page.cppm
        pages/button.cppm
        pages/forms_page.cppm
        pages/checkbox.cppm
        pages/switch.cppm
        pages/flow.cppm
        pages/card.cppm
        pages/tabs.cppm
        pages/overlay.cppm
        pages/dialog.cppm
        pages/select.cppm
)

if (CMAKE_HOST_SYSTEM_NAME MATCHES "Linux")
    message("current platform: Linux ")
    add_executable(${PROJECT_NAME} ${PROJECT_CPP_FILES})

elseif (CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
    message("current platform: Windows ")
    add_executable(${PROJECT_NAME} WIN32 ${PROJECT_CPP_FILES})
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
else ()
    message("current platform: unknown ")
    add_executable(${PROJECT_NAME} ${PROJECT_CPP_FILES})
endif ()


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
