import QtQuick
import QtQuick.Controls as QQC
import Nandina.Theme
import Nandina.Color
import Nandina.Tokens
import "../theme_utils.js" as ThemeUtils
import "input_validation_utils.js" as InputValidationUtils

Item {
    id: root

    enum InputType {
        Text, //默认的普通纯文本输入
        Password, //密码输入，显示为圆点
        Email, //邮箱输入
        Url, //网址输入
        Telephone, //电话号码输入
        Number,  //数字输入，允许小数点和负号
        Search //搜索输入，通常带有放大镜图标
    }

    enum Style {
        Outlined,
        Underline
    }

    enum ValidateTrigger {
        OnInput,
        OnBlur,
        OnSubmit
    }

    implicitWidth: 260
    implicitHeight: textField.implicitHeight + (hintText.visible ? hintText.implicitHeight + root.verticalSpacing : 0)

    property alias text: textField.text // 直接暴露 text 属性，方便使用
    property alias placeholderText: textField.placeholderText // 直接暴露 placeholderText 属性，方便使用
    property int maxLength: textField.maximumLength //输入框最大长度，默认为 32767
    property int inputType: NanInput.InputType.Text // 输入框类型，默认为普通文本输入
    property int style: NanInput.Style.Outlined // 输入框样式，默认为 Outlined
    property int validateTrigger: NanInput.ValidateTrigger.OnInput // 校验触发时机，默认为输入时校验
    property bool disabled: false // 是否禁用输入框，默认为 false
    property bool readOnly: false // 是否只读，默认为 false
    property bool invalid: false // 是否显示为无效状态，默认为 false
    property bool success: false // 是否显示为成功状态，默认为 false
    property bool clearable: false // 是否显示清除按钮，默认为 false
    property bool showPasswordToggle: true // 密码输入时是否显示切换明文按钮，默认为 true
    property bool passwordVisible: false // 密码输入时是否显示明文，默认为 false
    property string errorText: "" // 错误提示文本
    property string helperText: "" // 帮助文本
    property bool enableAutoValidation: true // 是否自动触发校验，默认为 true
    property bool skipEmptyValidation: true // 是否跳过空文本的校验，默认为 true
    property string defaultValidationErrorText: "输入格式不正确" // 默认的内置校验错误提示文本
    property string defaultCustomValidationErrorText: "输入不符合要求" // 默认的自定义校验错误提示文本
    property bool useCustomValidator: false // 是否使用自定义校验函数，默认为 false
    property font textFont: NanTypography.body
    property font helperFont: NanTypography.caption
    property int verticalSpacing: NanSpacing.sm
    property int horizontalPadding: NanSpacing.sm

    // 自定义校验函数，接受当前输入值和输入组件实例作为参数，返回一个对象 { valid: bool, message: string }
    property var validatorFn: function (_value, _input) {
        return true;
    }
    property var themeManager: null // 允许外部传入 ThemeManager 实例以使用主题色，默认为 null 将自动查找父级 ThemeManager

    readonly property int textType: NanInput.InputType.Text
    readonly property int passwordType: NanInput.InputType.Password
    readonly property int emailType: NanInput.InputType.Email
    readonly property int urlType: NanInput.InputType.Url
    readonly property int telephoneType: NanInput.InputType.Telephone
    readonly property int numberType: NanInput.InputType.Number
    readonly property int searchType: NanInput.InputType.Search

    readonly property int outlinedStyle: NanInput.Style.Outlined
    readonly property int underlineStyle: NanInput.Style.Underline
    readonly property int validateOnInput: NanInput.ValidateTrigger.OnInput
    readonly property int validateOnBlur: NanInput.ValidateTrigger.OnBlur
    readonly property int validateOnSubmit: NanInput.ValidateTrigger.OnSubmit

    readonly property bool hovered: hoverArea.containsMouse
    readonly property bool pressed: hoverArea.pressed
    readonly property bool focused: textField.activeFocus
    readonly property bool entered: hovered
    readonly property bool exited: !hovered
    readonly property bool hasText: root.text.length > 0
    readonly property bool showClearAction: root.clearable && root.hasText && !root.disabled && !root.readOnly
    readonly property bool showRevealAction: root.inputType === NanInput.InputType.Password && root.showPasswordToggle && !root.disabled
    readonly property bool hasTrailingAction: root.showClearAction || root.showRevealAction

    readonly property var builtInValidationResult: InputValidationUtils.validateBuiltIn({
        value: root.text,
        inputType: root.inputType,
        constants: {
            emailType: NanInput.InputType.Email,
            urlType: NanInput.InputType.Url,
            numberType: NanInput.InputType.Number
        }
    })

    readonly property var customValidationResult: InputValidationUtils.validateCustom({
        value: root.text,
        input: root,
        useCustomValidator: root.useCustomValidator,
        validatorFn: root.validatorFn,
        defaultCustomValidationErrorText: root.defaultCustomValidationErrorText
    })

    property bool hasValidationBeenBlurred: false
    property bool hasValidationBeenSubmitted: false

    readonly property bool autoValidationActivated: {
        if (!root.enableAutoValidation)
            return false;
        if (root.validateTrigger === NanInput.ValidateTrigger.OnBlur)
            return root.hasValidationBeenBlurred;
        if (root.validateTrigger === NanInput.ValidateTrigger.OnSubmit)
            return root.hasValidationBeenSubmitted;
        return true;
    }

    readonly property bool builtInInvalid: root.autoValidationActivated && (!root.skipEmptyValidation || root.hasText) && !root.builtInValidationResult.valid

    readonly property bool customInvalid: root.autoValidationActivated && (!root.skipEmptyValidation || root.hasText) && !root.customValidationResult.valid

    readonly property bool effectiveInvalid: root.invalid || root.builtInInvalid || root.customInvalid

    readonly property string autoValidationErrorText: {
        if (root.builtInInvalid)
            return root.builtInValidationResult.message;
        if (root.customInvalid)
            return root.customValidationResult.message;
        return "";
    }

    readonly property string resolvedErrorText: {
        if (root.errorText.length > 0)
            return root.errorText;
        return root.autoValidationErrorText;
    }

    readonly property bool effectiveSuccess: root.success && !root.effectiveInvalid

    ThemeManager {
        id: fallbackThemeManager
    }

    readonly property var resolvedThemeManager: ThemeUtils.resolveThemeManager(root, root.themeManager, fallbackThemeManager)

    readonly property var themePalette: root.resolvedThemeManager && root.resolvedThemeManager.currentPaletteCollection ? root.resolvedThemeManager.currentPaletteCollection : null

    signal accepted
    signal validationChanged(bool valid, string message)

    function validateNow() {
        return {
            valid: !root.effectiveInvalid,
            message: root.resolvedErrorText
        };
    }

    function requestValidation() {
        if (root.validateTrigger === NanInput.ValidateTrigger.OnBlur)
            root.hasValidationBeenBlurred = true;
        if (root.validateTrigger === NanInput.ValidateTrigger.OnSubmit)
            root.hasValidationBeenSubmitted = true;
        root.emitValidationState();
        return root.validateNow();
    }

    function resetValidationState() {
        root.hasValidationBeenBlurred = false;
        root.hasValidationBeenSubmitted = false;
        root.emitValidationState();
    }

    function emitValidationState() {
        const state = root.validateNow();
        root.validationChanged(state.valid, state.message);
    }

    QQC.TextField {
        id: textField
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        activeFocusOnTab: !root.disabled
        enabled: !root.disabled
        readOnly: root.readOnly
        rightPadding: root.hasTrailingAction ? trailingActions.implicitWidth + root.horizontalPadding + 2 : root.horizontalPadding
        font: root.textFont
        echoMode: {
            if (root.inputType !== NanInput.InputType.Password)
                return TextInput.Normal;
            return root.passwordVisible ? TextInput.Normal : TextInput.Password;
        }
        inputMethodHints: {
            if (root.inputType === NanInput.InputType.Email)
                return Qt.ImhEmailCharactersOnly;
            if (root.inputType === NanInput.InputType.Url)
                return Qt.ImhUrlCharactersOnly;
            if (root.inputType === NanInput.InputType.Number)
                return Qt.ImhFormattedNumbersOnly;
            if (root.inputType === NanInput.InputType.Telephone)
                return Qt.ImhDialableCharactersOnly;
            if (root.inputType === NanInput.InputType.Password)
                return Qt.ImhSensitiveData | Qt.ImhNoPredictiveText;
            return Qt.ImhNone;
        }
        color: root.themePalette ? root.themePalette.color0 : root.themePalette.onAccent
        placeholderTextColor: root.themePalette ? root.themePalette.subHeadlines1 : "#a3a3b2"
        selectByMouse: true

        background: Item {
            Rectangle {
                anchors.fill: parent
                visible: root.style === NanInput.Style.Outlined
                radius: 8
                border.width: textField.activeFocus ? 2 : 1
                border.color: {
                    if (root.effectiveInvalid)
                        return root.themePalette ? root.themePalette.error : "#d9534f";
                    if (root.effectiveSuccess)
                        return root.themePalette ? root.themePalette.success : "#55b16c";
                    if (textField.activeFocus)
                        return root.themePalette ? root.themePalette.activeBorder : "#4f8cff";
                    return root.themePalette ? root.themePalette.inactiveBorder : "#666";
                }
                color: root.themePalette ? root.themePalette.secondaryPane : "#2b2b33"
                opacity: root.disabled ? 0.55 : 1.0

                Behavior on border.color {
                    ColorAnimation {
                        duration: 120
                    }
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                visible: root.style === NanInput.Style.Underline
                height: textField.activeFocus ? 2 : 1
                radius: 1
                color: {
                    if (root.effectiveInvalid)
                        return root.themePalette ? root.themePalette.error : "#d9534f";
                    if (root.effectiveSuccess)
                        return root.themePalette ? root.themePalette.success : "#55b16c";
                    if (textField.activeFocus)
                        return root.themePalette ? root.themePalette.activeBorder : "#4f8cff";
                    return root.themePalette ? root.themePalette.inactiveBorder : "#666";
                }
                opacity: root.disabled ? 0.55 : 1.0

                Behavior on color {
                    ColorAnimation {
                        duration: 120
                    }
                }
            }
        }

        onAccepted: {
            if (root.validateTrigger === NanInput.ValidateTrigger.OnSubmit)
                root.hasValidationBeenSubmitted = true;
            root.emitValidationState();
            root.accepted();
        }

        onActiveFocusChanged: {
            if (!activeFocus && root.validateTrigger === NanInput.ValidateTrigger.OnBlur) {
                root.hasValidationBeenBlurred = true;
                root.emitValidationState();
            }
        }

        onTextChanged: {
            if (root.validateTrigger === NanInput.ValidateTrigger.OnInput)
                root.emitValidationState();
            else if (!root.enableAutoValidation)
                root.emitValidationState();
        }
    }

    Item {
        id: trailingActions
        anchors.right: textField.right
        anchors.rightMargin: root.verticalSpacing
        anchors.verticalCenter: textField.verticalCenter
        implicitWidth: actionRow.implicitWidth
        implicitHeight: actionRow.implicitHeight
        visible: root.hasTrailingAction
        z: 2

        Row {
            id: actionRow
            spacing: NanSpacing.xs

            Rectangle {
                width: 20
                height: 20
                radius: 10
                visible: root.showClearAction
                color: root.themePalette ? root.themePalette.overlay0 : "#666"
                opacity: 0.32

                Text {
                    anchors.centerIn: parent
                    text: "×"
                    color: root.themePalette ? root.themePalette.mainHeadline : "#f5f5f5"
                    font: NanTypography.body
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: parent.visible
                    onClicked: {
                        textField.clear();
                        textField.forceActiveFocus();
                        root.emitValidationState();
                    }
                }
            }

            Rectangle {
                width: 26
                height: 20
                radius: 10
                visible: root.showRevealAction
                color: root.themePalette ? root.themePalette.overlay0 : "#666"
                opacity: 0.28

                Text {
                    anchors.centerIn: parent
                    text: root.passwordVisible ? "隐藏" : "显示"
                    color: root.themePalette ? root.themePalette.mainHeadline : "#f5f5f5"
                    font: NanTypography.caption
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: parent.visible
                    onClicked: {
                        root.passwordVisible = !root.passwordVisible;
                        textField.forceActiveFocus();
                    }
                }
            }
        }
    }

    MouseArea {
        id: hoverArea
        anchors.fill: textField
        enabled: !root.disabled
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
    }

    Text {
        id: hintText
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: textField.bottom
        anchors.topMargin: root.verticalSpacing
        visible: root.resolvedErrorText.length > 0 || root.helperText.length > 0
        text: root.resolvedErrorText.length > 0 ? root.resolvedErrorText : root.helperText
        color: root.resolvedErrorText.length > 0 ? (root.themePalette ? root.themePalette.error : "#d9534f") : (root.themePalette ? root.themePalette.subHeadlines1 : "#a3a3b2")
        font: root.helperFont
        wrapMode: Text.Wrap
    }

    onInputTypeChanged: root.emitValidationState()
    onInvalidChanged: root.emitValidationState()
    onErrorTextChanged: root.emitValidationState()
    onValidatorFnChanged: root.emitValidationState()
    onValidateTriggerChanged: root.resetValidationState()
}
