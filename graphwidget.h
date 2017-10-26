#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include "panel3d.h"
#include "graph2d.h"
#include "objnetdevice.h"
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>

using namespace Objnet;

class GraphWidget : public QWidget
{
    Q_OBJECT

private:
    QPanel3D *mScene;
    Graph2D *mGraph;
    QElapsedTimer mTimer;
    QMap<unsigned long, QString> mDevices; // serial -> name[mac]
    QMap<unsigned long, QStringList> mVarNames;
    QFormLayout *mNamesLay;

    QPushButton *mClearBtn;

    void setupScene();

    void addPoint(QString name, float val);
    void removeGraph(QString name);

    void addObjname(unsigned long serial, QString objname);
    void removeObjname(unsigned long serial, QString objname);
    int getRow(unsigned long serial, QString objname);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

public:
    explicit GraphWidget(QWidget *parent = 0);

signals:
    void periodChanged(unsigned long serial, QString objname, int period_ms);

public slots:
    void updateObject(QString name, QVariant value);
};

#endif // GRAPHWIDGET_H