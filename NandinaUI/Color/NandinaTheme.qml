import QtQuick

QtObject {
    id: root

    property string currentTheme: NandinaThemeType.latte
     
    signal themeChanged(string theme)
    
    // 动态加载的主题组件
    property QtObject themeColors: null
    
    // 主题加载器
    property Loader themeLoader: Loader {
        sourceComponent: {
            switch (root.currentTheme) {
                case NandinaThemeType.latte:
                    return latteComponent
                case NandinaThemeType.frappe:
                    return frappeComponent
                case NandinaThemeType.macchiato:
                    return macchiatoComponent
                case NandinaThemeType.mocha:
                    return mochaComponent
                default:
                    return latteComponent
            }
        }
        onLoaded: {
            themeColors = item
        }
    }
    
    // 主题组件定义
    Component {
        id: latteComponent
        CatppuccinLatte {}
    }
    
    Component {
        id: frappeComponent
        CatppuccinFrappe {}
    }
    
    Component {
        id: macchiatoComponent
        CatppuccinMacchiato {}
    }
    
    Component {
        id: mochaComponent
        CatppuccinMocha {}
    }
    
    onCurrentThemeChanged: {
        themeChanged(currentTheme)
    }
    
    // 代理所有颜色属性
    readonly property color rosewater: themeColors ? themeColors.rosewater : "#000000"
    readonly property color flamingo: themeColors ? themeColors.flamingo : "#000000"
    readonly property color pink: themeColors ? themeColors.pink : "#000000"
    readonly property color mauve: themeColors ? themeColors.mauve : "#000000"
    readonly property color red: themeColors ? themeColors.red : "#000000"
    readonly property color maroon: themeColors ? themeColors.maroon : "#000000"
    readonly property color peach: themeColors ? themeColors.peach : "#000000"
    readonly property color yellow: themeColors ? themeColors.yellow : "#000000"
    readonly property color green: themeColors ? themeColors.green : "#000000"
    readonly property color teal: themeColors ? themeColors.teal : "#000000"
    readonly property color sky: themeColors ? themeColors.sky : "#000000"
    readonly property color sapphire: themeColors ? themeColors.sapphire : "#000000"
    readonly property color blue: themeColors ? themeColors.blue : "#000000"
    readonly property color lavender: themeColors ? themeColors.lavender : "#000000"
    readonly property color text: themeColors ? themeColors.text : "#000000"
    readonly property color subtext1: themeColors ? themeColors.subtext1 : "#000000"
    readonly property color subtext0: themeColors ? themeColors.subtext0 : "#000000"
    readonly property color overlay2: themeColors ? themeColors.overlay2 : "#000000"
    readonly property color overlay1: themeColors ? themeColors.overlay1 : "#000000"
    readonly property color overlay0: themeColors ? themeColors.overlay0 : "#000000"
    readonly property color surface2: themeColors ? themeColors.surface2 : "#000000"
    readonly property color surface1: themeColors ? themeColors.surface1 : "#000000"
    readonly property color surface0: themeColors ? themeColors.surface0 : "#000000"
    readonly property color base: themeColors ? themeColors.base : "#000000"
    readonly property color mantle: themeColors ? themeColors.mantle : "#000000"
    readonly property color crust: themeColors ? themeColors.crust : "#000000"
}
