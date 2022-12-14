#ifndef OBJTABLE_H
#define OBJTABLE_H

#include <QTableWidget>
#include <QPushButton>
#include <QDrag>
#include <QMimeData>
#include <QVector3D>
#include <QQuaternion>
#include "objnetdevice.h"

using namespace Objnet;

class ObjTable : public QTableWidget
{
    Q_OBJECT

private:
    ObjnetDevice *mDevice;
//    uint32_t mOldTimestamp;

    QString valueToString(QVariant value);

protected:
    void startDrag(Qt::DropActions supportedActions);

private slots:
    void onCellChanged(int row, int col);
    void onCellDblClick(int row, int col);
    void updateTable();

public:
    ObjTable(QWidget *parent = nullptr);

    void setDevice(ObjnetDevice *dev);

public slots:
    void updateObject(QString name, QVariant value);
    void updateTimedObject(QString name, uint32_t timestamp, QVariant value);
};

#endif // OBJTABLE_H
