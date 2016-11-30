#include "graphwidget.h"

GraphWidget::GraphWidget(QWidget *parent) : QWidget(parent, Qt::Window)
{
    mScene = new QPanel3D(this);
    mScene->setMinimumSize(320, 240);
    mGraph = new Graph2D(mScene->root());
    setupScene();

    mClearBtn = new QPushButton("Clear");
    connect(mClearBtn, &QPushButton::clicked, [=](){mGraph->clear(); mTimer.restart();});

    QGridLayout *lay = new QGridLayout;
    lay->addWidget(mClearBtn, 0, 0);
    lay->addWidget(mScene, 1, 0);
    setLayout(lay);
}

void GraphWidget::setupScene()
{
    mScene->camera()->setPosition(QVector3D(0, 0, 10));
    mScene->camera()->setDirection(QVector3D(0, 0, -1));
    mScene->camera()->setTopDir(QVector3D(0, 1, 0));
    mScene->camera()->setOrtho(true);
    mScene->camera()->setFixedViewportSize(QSizeF(110, 110));
    mScene->camera()->setFixedViewport(true);
    mScene->setBackColor(Qt::white);
    mScene->setLightingEnabled(false);

    mGraph->setSize(100, 100);
    mGraph->setPosition(-50, -50, 0);
    mGraph->setBounds(QRectF(0, 0, 5, 0));
}

void GraphWidget::addPoint(QString name, float val)
{
    if (!mTimer.isValid())
        mTimer.start();

    float time = mTimer.elapsed() * 0.001f;
    mGraph->addPoint(name, time, val);

    mScene->updateGL();
}

void GraphWidget::removeGraph(QString name)
{
    mGraph->clear(name);
}
