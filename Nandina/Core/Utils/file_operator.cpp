//
// Created by cvrain on 2025/11/2.
//

#include "file_operator.hpp"

#include <QFile>

namespace Nandina::Core::Utils {
    std::optional<QJsonDocument> FileOperator::readJsonFile(const QString &filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return std::nullopt;
        }
        return QJsonDocument::fromJson(file.readAll());
    }
}
