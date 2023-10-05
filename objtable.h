#ifndef OBJTABLE_H
#define OBJTABLE_H

#include <QTreeView>
#include <QStandardItemModel>
#include <QPushButton>
#include <QDrag>
#include <QMimeData>
#include <QVector3D>
#include <QQuaternion>
#include "objnetdevice.h"

using namespace Objnet;

class ObjTable : public QTreeView
{
    Q_OBJECT

private:
    ObjnetDevice *m_device;
    QStandardItemModel m_model;
    bool m_flag;

    QString valueToString(QVariant value);
    QVariant valueFromString(QString s, ObjectInfo::Type t);

    QList<QStandardItem *> createRow(ObjectInfo *info);
    void updateItem(QStandardItem *parent, QString name, QVariant value);
    QVariant readItem(ObjectInfo *info, QStandardItem *item);

protected:
    void startDrag(Qt::DropActions supportedActions) override;

private slots:
    void onClick(const QModelIndex &index);
    void itemChanged(QStandardItem *item);
    void updateTable();

public:
    ObjTable(QWidget *parent = nullptr);

    void setDevice(ObjnetDevice *dev);

public slots:
    void updateObject(QString name, QVariant value);
    void updateTimedObject(QString name, uint32_t timestamp, QVariant value);
};

#endif // OBJTABLE_H
