//
// Created by cvrain on 2025/11/2.
//

#ifndef TRYNANDINA_FILE_OPERATOR_HPP
#define TRYNANDINA_FILE_OPERATOR_HPP
#include <QJsonDocument>

namespace Nandina::Core::Utils {
    class FileOperator {
    public:
        static std::optional<QJsonDocument> readJsonFile(const QString &filePath);
    };
}


#endif //TRYNANDINA_FILE_OPERATOR_HPP
