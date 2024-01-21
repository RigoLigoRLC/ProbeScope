/*
    SPDX-FileCopyrightText: 1997, 1998 Lars Doelle <lars.doelle@on-line.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef HISTORYTYPENONE_H
#define HISTORYTYPENONE_H

#include "HistoryType.h"
#include "qtermwidget_export.h"

namespace Konsole {
    class QTERMWIDGET_EXPORT HistoryTypeNone : public HistoryType {
    public:
        HistoryTypeNone();

        bool isEnabled() const override;
        int maximumLineCount() const override;

        HistoryScroll *scroll(HistoryScroll *) const override;
    };

}

#endif
