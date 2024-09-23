
#pragma once

#include <QEvent>
#include <QHeaderView>
#include <QObject>


class FirstColumnFollowResizeFilter : public QObject {
    // Q_OBJECT
public:
    explicit FirstColumnFollowResizeFilter(QObject *parent = nullptr) : QObject(parent){};

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (event->type() == QEvent::Resize) {
            auto header = qobject_cast<QHeaderView *>(watched);
            if (!header) {
                return QObject::eventFilter(watched, event);
            }

            // Resize the first column to fill the header width
            header->resizeSection(0, header->width() - header->sectionSize(1));
        }
        return QObject::eventFilter(watched, event);
    }
};
