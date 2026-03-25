// PageRegistration.qml
// Singleton: declares all routes for the example app.
// Keys are plain strings — no integer enum needed.
pragma ComponentBehavior: Bound
pragma Singleton

import QtQuick
import Nandina.Controls

QtObject {
    id: root

    readonly property list<QtObject> pageRegistry: [
        NanRoute {
            key: "SurfacePressablePanelPage"
            section: "Components"
            navTitle: "Surface / Pressable / Panel"
            title: "Surface / Pressable / Panel 基础容器与交互原语"
            summary: "聚合展示基础容器与交互原语，便于观察状态反馈与组合方式。"
            iconText: "\uE3D0"
            pageSource: Qt.resolvedUrl("pages/SurfacePressablePanelPage.qml")
        },
        NanRoute {
            key: "LabelPage"
            section: "Components"
            navTitle: "Label"
            title: "Label 标签组件"
            summary: "可访问的表单标签，支持必填/错误/禁用状态，主题感知颜色适配。"
            iconText: "\uE893"
            pageSource: Qt.resolvedUrl("pages/LabelPage.qml")
        },
        NanRoute {
            key: "CardPage"
            section: "Components"
            navTitle: "Card"
            title: "Card 卡片组件"
            summary: "聚焦 Card 的结构分区、视觉预设与交互状态。"
            iconText: "\uE3FD"
            pageSource: Qt.resolvedUrl("pages/CardPage.qml")
        },
        NanRoute {
            key: "ButtonPage"
            section: "Components"
            navTitle: "Button"
            title: "Button 按钮组件"
            summary: "五种预设 × 七种色彩变体 × 三种尺寸，支持左右图标与键盘交互。"
            iconText: "\uE5C8"
            pageSource: Qt.resolvedUrl("pages/ButtonPage.qml")
        },
        NanRoute {
            key: "SideBarPage"
            section: "Components"
            navTitle: "SideBar"
            title: "SideBar 侧边栏组件"
            summary: "用于演示分组导航、折叠模式与侧边栏布局行为。折叠模式使用 NanSideBar.Icon / Offcanvas / None 枚举。"
            iconText: "\uE5C3"
            pageSource: Qt.resolvedUrl("pages/SideBarPage.qml")
        },
        NanRoute {
            key: "BadgePage"
            section: "Components"
            navTitle: "Badge"
            title: "Badge Component"
            summary: "小型内联标签，用于分类、状态标注或数字计数。无交互行为。"
            iconText: "🏷️"
            pageSource: Qt.resolvedUrl("pages/BadgePage.qml")
        },
        NanRoute {
            key: "SettingPage"
            section: "Footer"
            navTitle: "Settings"
            title: "Settings 设置页"
            summary: "页面基本设置。"
            iconText: "\uE8B8"
            pageSource: Qt.resolvedUrl("pages/SettingPage.qml")
        }
    ]

    function firstKey(): string {
        return root.pageRegistry.length > 0 ? root.pageRegistry[0].key : "";
    }
}
