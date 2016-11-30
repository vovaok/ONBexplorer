#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H

#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include "panel3d.h"
#include "graph2d.h"

class GraphWidget : public QWidget
{
    Q_OBJECT

private:
    QPanel3D *mScene;
    Graph2D *mGraph;
    QElapsedTimer mTimer;

    QPushButton *mClearBtn;

    void setupScene();

public:
    explicit GraphWidget(QWidget *parent = 0);

    void addPoint(QString name, float val);
    void removeGraph(QString name);

signals:

public slots:
};

#endif // GRAPHWIDGET_H
