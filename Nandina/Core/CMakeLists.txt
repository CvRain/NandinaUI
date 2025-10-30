# Core 模块 - 枚举、常量、类型定义
set(CORE_QML_FILES

)

qt_add_library(NandinaCore STATIC)
qt_add_qml_module(NandinaCore
        URI Nandina.Core
        VERSION 1.0
        QML_FILES ${CORE_QML_FILES}
)

target_link_libraries(NandinaCore
        PRIVATE Qt6::Quick
)