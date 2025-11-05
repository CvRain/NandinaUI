//
// Created by cvrain on 2025/11/4.
//

#ifndef TRYNANDINA_JSON_PARSER_HPP
#define TRYNANDINA_JSON_PARSER_HPP

#include <QJsonObject>
#include <QString>

#include "baseColors.hpp"

namespace Nandina::Core::Utils::JsonParser {
    template<typename T>
    T parser(const QJsonObject &json) {
        return parser<QString>(json);
    }

    template<>
    QString parser<QString>(const QJsonObject &json);

    template<>
    BaseColors parser<BaseColors>(const QJsonObject &json);

}

#endif //TRYNANDINA_JSON_PARSER_HPP
