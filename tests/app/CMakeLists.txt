nandina_add_test(
    NAME    test_app_authoring
    SOURCES test_app_authoring.cpp
    DEPS
        nandina::app
)

nandina_add_test(
    NAME    test_app_navigation
    SOURCES test_app_navigation.cpp
    DEPS
        nandina::app
)

nandina_add_test(
    NAME    test_app_authoring_qol
    SOURCES test_app_authoring_qol.cpp
    DEPS
        nandina::app
)