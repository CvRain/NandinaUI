//
// Created by CvRai on 2025/11/2.
//

#include "colorCollection.hpp"

namespace Nandina::Core::Types {
    CatppuccinSetting::CatppuccinSetting(QObject *parent)
        : QObject(parent) {
    }

    QString CatppuccinSetting::catppuccinTypeToString(const CatppuccinType type) {
        switch (type) {
            case CatppuccinType::Latte:
                return "Latte";
            case CatppuccinType::Frappe:
                return "Frappe";
            case CatppuccinType::Macchiato:
                return "Macchiato";
            case CatppuccinType::Mocha:
                return "Mocha";
        }
        return "Latte";
    }

    CatppuccinSetting::CatppuccinType CatppuccinSetting::stringToCatppuccinType(const QString &name) {
        //将名字转换为全小写
        const auto lowerName = name.toLower();

        if (lowerName == "latte") {
            return CatppuccinType::Latte;
        }
        if (lowerName == "frappe") {
            return CatppuccinType::Frappe;
        }
        if (lowerName == "macchiato") {
            return CatppuccinType::Macchiato;
        }
        if (lowerName == "mocha") {
            return CatppuccinType::Mocha;
        }
        return CatppuccinType::Latte;
    }
}
