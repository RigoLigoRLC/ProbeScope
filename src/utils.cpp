
#include "utils.h"

QString ProbeScopeUtil::bytesToSize(size_t s, int prec) {
    if (s > (1 << 30))
        return QString::number(double(s) / (1 << 30), 'f', prec) + " GB";
    if (s > (1 << 20))
        return QString::number(double(s) / (1 << 20), 'f', prec) + " MB";
    if (s > (1 << 10))
        return QString::number(double(s) / (1 << 10), 'f', prec) + " KB";
    return QString::number(s) + " B";
}
