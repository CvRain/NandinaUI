//
// Created by cvrain on 2025/11/4.
//

#include "json_parser.hpp"

namespace Nandina::Core::Utils::JsonParser {
    template<>
    QString parser<QString>(const QJsonObject &json) {
        // 非常简单的将整个json object转换为字符串
        return json.value("value").toString();
    }

    template<>
    BaseColors parser<BaseColors>(const QJsonObject &json) {
        BaseColors newBaseColors;
        newBaseColors.rosewater = json["rosewater"].toString();
        newBaseColors.flamingo = json["flamingo"].toString();
        newBaseColors.pink = json["pink"].toString();
        newBaseColors.mauve = json["mauve"].toString();
        newBaseColors.red = json["red"].toString();
        newBaseColors.maroon = json["maroon"].toString();
        newBaseColors.peach = json["peach"].toString();
        newBaseColors.yellow = json["yellow"].toString();
        newBaseColors.green = json["green"].toString();
        newBaseColors.teal = json["teal"].toString();
        newBaseColors.sky = json["sky"].toString();
        newBaseColors.sapphire = json["sapphire"].toString();
        newBaseColors.blue = json["blue"].toString();
        newBaseColors.lavender = json["lavender"].toString();
        newBaseColors.text = json["text"].toString();
        newBaseColors.subtext1 = json["subtext1"].toString();
        newBaseColors.subtext0 = json["subtext0"].toString();
        newBaseColors.overlay2 = json["overlay2"].toString();
        newBaseColors.overlay1 = json["overlay1"].toString();
        newBaseColors.overlay0 = json["overlay0"].toString();
        newBaseColors.surface2 = json["surface2"].toString();
        newBaseColors.surface1 = json["surface1"].toString();
        newBaseColors.surface0 = json["surface0"].toString();
        newBaseColors.base = json["base"].toString();
        newBaseColors.mantle = json["mantle"].toString();
        newBaseColors.crust = json["crust"].toString();
        return newBaseColors;
    }
}
