
#pragma once

#include "psprobe/bridge.h"
#include <QString>


namespace probelib {
    inline QString operator<<(QString &qstr, psprobe_ext_dyn_str &str) {
        qstr += QString::fromUtf8(str.str, str.len);
        psprobe_extdynstr_dispose(&str);
        return qstr;
    }

    inline QString operator<<(QString &&qstr, psprobe_ext_dyn_str &str) {
        return qstr << str;
    }
}
