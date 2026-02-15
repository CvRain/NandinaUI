function validateBuiltIn(args) {
    var value = args.value || ""
    if (value.length === 0) {
        return {
            valid: true,
            message: ""
        }
    }

    if (args.inputType === args.constants.emailType) {
        var emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/
        return {
            valid: emailRegex.test(value),
            message: "请输入有效的 Email 地址"
        }
    }

    if (args.inputType === args.constants.urlType) {
        var urlRegex = /^(https?:\/\/)?([\w-]+\.)+[\w-]{2,}(\/[\S]*)?$/i
        return {
            valid: urlRegex.test(value),
            message: "请输入有效的 URL"
        }
    }

    if (args.inputType === args.constants.numberType) {
        var numberRegex = /^-?\d+(\.\d+)?$/
        return {
            valid: numberRegex.test(value),
            message: "请输入有效数字"
        }
    }

    return {
        valid: true,
        message: ""
    }
}

function validateCustom(args) {
    if (!args.useCustomValidator || typeof args.validatorFn !== "function") {
        return {
            valid: true,
            message: ""
        }
    }

    var customResult = args.validatorFn(args.value, args.input)

    if (typeof customResult === "boolean") {
        return {
            valid: customResult,
            message: customResult ? "" : args.defaultCustomValidationErrorText
        }
    }

    if (typeof customResult === "string") {
        return {
            valid: customResult.length === 0,
            message: customResult
        }
    }

    if (customResult && typeof customResult === "object") {
        var isValid = customResult.valid !== false
        return {
            valid: isValid,
            message: isValid ? "" : (customResult.message || args.defaultCustomValidationErrorText)
        }
    }

    return {
        valid: true,
        message: ""
    }
}
