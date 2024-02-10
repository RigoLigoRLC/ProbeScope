
#pragma once

#include <QString>

namespace ProbeScopeUtil {
    /**
     * @brief Convert file size in bytes into human readable size string with a certain precision.
     *
     * @param s size in bytes
     * @param prec precision, digits of decimal places
     * @return QString size string
     */
    QString bytesToSize(size_t s, int prec);
}
