nandina_add_test(
    NAME    test_widgets_button
    SOURCES test_widgets_button.cpp
    DEPS
        nandina::widgets
        nandina::theme
)

nandina_add_test(
    NAME    test_widgets_label
    SOURCES test_widgets_label.cpp
    DEPS
        nandina::widgets
        nandina::theme
)

nandina_add_test(
    NAME    test_widgets_panel
    SOURCES test_widgets_panel.cpp
    DEPS
        nandina::widgets
)

nandina_add_test(
    NAME    test_widgets_text_field
    SOURCES test_widgets_text_field.cpp
    DEPS
        nandina::widgets
        nandina::theme
)

nandina_add_test(
    NAME    test_widgets_field
    SOURCES test_widgets_field.cpp
    DEPS
        nandina::widgets
        nandina::theme
)
