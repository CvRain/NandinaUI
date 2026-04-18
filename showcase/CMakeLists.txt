cmake_minimum_required(VERSION 4.0)
project(nandina_showcase LANGUAGES CXX)

# Showcase — 展示与验证应用
# 职责：以可运行应用验证组件、布局与主题能力，不作为框架核心依赖。
# 依赖：nandina::app（待实现后解注释）

set(PROJECT_CPP_FILES
        main.cpp
)

set(PROJECT_CPP_MODULE_FILES
        application.cppm
)

add_executable(${PROJECT_NAME} ${PROJECT_CPP_FILES})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_26)

target_sources(${PROJECT_NAME}
        PUBLIC
        FILE_SET CXX_MODULES FILES ${PROJECT_CPP_MODULE_FILES}
)


# 待 nandina::app 实现后启用：
target_link_libraries(${PROJECT_NAME}
        PRIVATE
        nandina::app
)
