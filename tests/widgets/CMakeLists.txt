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
    NAME    test_widgets_text
    SOURCES test_widgets_text.cpp
    DEPS
        nandina::widgets
        nandina::theme
)

nandina_add_test(
    NAME    test_widgets_tag
    SOURCES test_widgets_tag.cpp
    DEPS
        nandina::widgets
        nandina::theme
)

nandina_add_test(
    NAME    test_widgets_checkbox
    SOURCES test_widgets_checkbox.cpp
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

nandina_add_test(
    NAME    test_widgets_progressbar
    SOURCES test_widgets_progressbar.cpp
    DEPS
        nandina::widgets
        nandina::theme
)
