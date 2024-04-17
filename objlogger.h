#ifndef OBJLOGGER_H
#define OBJLOGGER_H

#include <QtWidgets>
#include "objectinfo.h"
#include "objnetdevice.h"

class ObjLogger : public QWidget
{
    Q_OBJECT
public:
    explicit ObjLogger(QWidget *parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

signals:

private:
    QSpinBox *m_intervalSpin;
    QCheckBox *m_enableChk;
    QListWidget *m_list;
    QTimer *m_timer = nullptr;
    QElapsedTimer m_elapsed;

    Objnet::ObjnetDevice *m_device = nullptr;
    QString m_filename;

    void setLoggingEnabled(bool enabled);
    void sendRequest();
    void updateObjectGroup(QVariantMap values);
};

#endif // OBJLOGGER_H
