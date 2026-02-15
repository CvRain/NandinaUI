import QtQuick
import QtQuick.Controls as QQC
import Nandina.Theme
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
    implicitHeight: textField.implicitHeight + (hintText.visible ? hintText.implicitHeight + 6 : 0)

    property alias text: textField.text
    property alias placeholderText: textField.placeholderText
    property int maxLength: textField.maximumLength
    property int inputType: NanInput.InputType.Text
    property int style: NanInput.Style.Outlined
    property int validateTrigger: NanInput.ValidateTrigger.OnInput
    property bool disabled: false
    property bool readOnly: false
    property bool invalid: false
    property bool success: false
    property bool clearable: false
    property bool showPasswordToggle: true
    property bool passwordVisible: false
    property string errorText: ""
    property string helperText: ""
    property bool enableAutoValidation: true
    property bool skipEmptyValidation: true
    property string defaultValidationErrorText: "输入格式不正确"
    property string defaultCustomValidationErrorText: "输入不符合要求"
    property bool useCustomValidator: false
    property var validatorFn: function (_value, _input) {
        return true;
    }
    property var themeManager: null

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
        rightPadding: root.hasTrailingAction ? trailingActions.implicitWidth + 10 : 8
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
        color: root.themePalette ? root.themePalette.mainHeadline : "#f5f5f5"
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
        anchors.rightMargin: 6
        anchors.verticalCenter: textField.verticalCenter
        implicitWidth: actionRow.implicitWidth
        implicitHeight: actionRow.implicitHeight
        visible: root.hasTrailingAction
        z: 2

        Row {
            id: actionRow
            spacing: 4

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
                    font.pixelSize: 13
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
                    font.pixelSize: 10
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
        anchors.topMargin: 6
        visible: root.resolvedErrorText.length > 0 || root.helperText.length > 0
        text: root.resolvedErrorText.length > 0 ? root.resolvedErrorText : root.helperText
        color: root.resolvedErrorText.length > 0 ? (root.themePalette ? root.themePalette.error : "#d9534f") : (root.themePalette ? root.themePalette.subHeadlines1 : "#a3a3b2")
        font.pixelSize: 13
        wrapMode: Text.Wrap
    }

    onInputTypeChanged: root.emitValidationState()
    onInvalidChanged: root.emitValidationState()
    onErrorTextChanged: root.emitValidationState()
    onValidatorFnChanged: root.emitValidationState()
    onValidateTriggerChanged: root.resetValidationState()
}
